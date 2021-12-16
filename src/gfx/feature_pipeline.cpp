#include "feature_pipeline.hpp"
#include "bgfx/bgfx.h"
#include "drawable.hpp"
#include "enums.hpp"
#include "feature.hpp"
#include "fields.hpp"
#include "graph.hpp"
#include "magic_enum.hpp"
#include "mesh.hpp"
#include "shaderc.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include "strings.hpp"
#include "texture.hpp"
#include "utils.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/join.hpp>
#include <cctype>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace gfx {

struct CodeBlock {
	std::string name;
	std::string code;
	std::vector<NamedDependency> dependencies;

	CodeBlock() = default;
	CodeBlock(std::string name, std::string code = std::string(), const std::vector<NamedDependency> &dependencies = std::vector<NamedDependency>())
		: name(name), code(code), dependencies(dependencies) {}
};

bool resolveCodeBlocks(const std::vector<CodeBlock> &blocks, std::string &output, bool ignoreMissingDependencies = true) {
	std::unordered_map<std::string, size_t> nodeNames;
	for (size_t i = 0; i < blocks.size(); i++) {
		const CodeBlock &block = blocks[i];
		if (!block.name.empty())
			nodeNames.insert_or_assign(block.name, i);
	}

	auto resolveNodeIndex = [&](const std::string &name) -> const size_t * {
		auto it = nodeNames.find(name);
		if (it != nodeNames.end())
			return &it->second;
		return nullptr;
	};

	std::set<std::string> missingDependencies;
	graph::Graph graph;
	graph.nodes.resize(blocks.size());
	for (size_t i = 0; i < blocks.size(); i++) {
		const CodeBlock &block = blocks[i];
		for (auto &dep : block.dependencies) {
			if (dep.type == DependencyType::Before) {
				if (const size_t *depIndex = resolveNodeIndex(dep.name)) {
					graph.nodes[*depIndex].dependencies.push_back(i);
				} else if (!ignoreMissingDependencies) {
					missingDependencies.insert(dep.name);
				}
			} else {
				if (const size_t *depIndex = resolveNodeIndex(dep.name)) {
					graph.nodes[i].dependencies.push_back(*depIndex);
				} else if (!ignoreMissingDependencies) {
					missingDependencies.insert(dep.name);
				}
			}
		}
	}

	if (!ignoreMissingDependencies && missingDependencies.size() > 0) {
		return false;
	}

	std::vector<size_t> sorted;
	if (!graph::topologicalSort(graph, sorted))
		return false;

	for (size_t i = 0; i < sorted.size(); i++) {
		output += blocks[sorted[i]].code;
		if ((i + 1) < sorted.size()) {
			output += "\n";
		}
	}
	return true;
}

static const char *bgfxAttribToShaderSemantic(bgfx::Attrib::Enum tag) {
	switch (tag) {
	case bgfx::Attrib::Position:
		return "POSITION";
	case bgfx::Attrib::Normal:
		return "NORMAL";
	case bgfx::Attrib::Tangent:
		return "TANGENT";
	case bgfx::Attrib::Bitangent:
		return "BITANGENT";
	case bgfx::Attrib::Color0:
		return "COLOR";
	case bgfx::Attrib::Color1:
		return "COLOR";
	case bgfx::Attrib::Color2:
		return "COLOR";
	case bgfx::Attrib::Color3:
		return "COLOR";
	case bgfx::Attrib::Indices:
		return "BLENDINDICES";
	case bgfx::Attrib::Weight:
		return "BLENDWEIGHT";
	case bgfx::Attrib::TexCoord0:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord1:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord2:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord3:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord4:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord5:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord6:
		return "TEXCOORD";
	case bgfx::Attrib::TexCoord7:
		return "TEXCOORD";
	default:
		return nullptr;
	}
}

static bgfx::Attrib::Enum allocateAttribTag(std::set<bgfx::Attrib::Enum> &set, const std::string &fieldName) {
	bgfx::Attrib::Enum result = bgfx::Attrib::Count;

	auto tryAllocateFrom = [&](const std::vector<bgfx::Attrib::Enum> &options) {
		for (const bgfx::Attrib::Enum &tag : options) {
			if (set.find(tag) == set.end()) {
				set.insert(tag);
				result = tag;
				return true;
			}
		}
		return false;
	};

	// Other than the position tag, these are merelty for convenience when viewed in graphics debuggers/shader sources
	if (containsCaseInsensitive(fieldName, "position")) {
		static std::vector<bgfx::Attrib::Enum> positionTags = {bgfx::Attrib::Position};
		if (tryAllocateFrom(positionTags))
			return result;
	} else if (containsCaseInsensitive(fieldName, "bitangent")) {
		static std::vector<bgfx::Attrib::Enum> tags = {bgfx::Attrib::Bitangent};
		if (tryAllocateFrom(tags))
			return result;
	} else if (containsCaseInsensitive(fieldName, "tangent")) {
		static std::vector<bgfx::Attrib::Enum> tags = {bgfx::Attrib::Tangent};
		if (tryAllocateFrom(tags))
			return result;
	} else if (containsCaseInsensitive(fieldName, "normal")) {
		static std::vector<bgfx::Attrib::Enum> tags = {bgfx::Attrib::Normal};
		if (tryAllocateFrom(tags))
			return result;
	} else if (containsCaseInsensitive(fieldName, "tex")) {
		static std::vector<bgfx::Attrib::Enum> tags = {bgfx::Attrib::TexCoord0, bgfx::Attrib::TexCoord1, bgfx::Attrib::TexCoord2, bgfx::Attrib::TexCoord3,
													   bgfx::Attrib::TexCoord4, bgfx::Attrib::TexCoord5, bgfx::Attrib::TexCoord6, bgfx::Attrib::TexCoord7};
		if (tryAllocateFrom(tags))
			return result;
	} else if (containsCaseInsensitive(fieldName, "color")) {
		static std::vector<bgfx::Attrib::Enum> tags = {bgfx::Attrib::Color0, bgfx::Attrib::Color1, bgfx::Attrib::Color2, bgfx::Attrib::Color3};
		if (tryAllocateFrom(tags))
			return result;
	}

	static std::vector<bgfx::Attrib::Enum> allTags = {
		bgfx::Attrib::TexCoord0, bgfx::Attrib::TexCoord1, bgfx::Attrib::TexCoord2, bgfx::Attrib::TexCoord3, bgfx::Attrib::TexCoord4,
		bgfx::Attrib::TexCoord5, bgfx::Attrib::TexCoord6, bgfx::Attrib::TexCoord7, bgfx::Attrib::Color0,	bgfx::Attrib::Color1,
		bgfx::Attrib::Color2,	 bgfx::Attrib::Color3,	  bgfx::Attrib::Normal,	   bgfx::Attrib::Tangent,	bgfx::Attrib::Bitangent,
	};
	tryAllocateFrom(allTags);
	return result;
}

struct VertexAttribute {
	bgfx::AttribType type;
};

static const char *l_preFeature = "preFeature";
static const char *l_postFeature = "postFeature";
static const char *l_writeOutputs = "writeOutputs";

struct InOutVar {
	std::string name;
	std::string type;
	std::string baseName;
	std::string initializer;
	InOutVar(std::string name, std::string type, std::string baseName, std::string initializer = std::string())
		: name(name), type(type), baseName(baseName), initializer(initializer) {}
};

struct ShaderStageBuilder {
	// inserted into root scope
	std::vector<CodeBlock> featureCodeSegments;

	// inserted into main() function scope
	std::vector<CodeBlock> mainCodeSegments;

	std::vector<InOutVar> inputs;
	std::vector<InOutVar> outputs;

	std::string materialStructBody;
	std::string materialStructInitializer;

	size_t counter = 0;

	ShaderStageBuilder() { init(); }

	void init() {
		CodeBlock &preFeature = mainCodeSegments.emplace_back(l_preFeature);
		preFeature.code = "// pre-feature";

		CodeBlock &postFeature = mainCodeSegments.emplace_back(l_postFeature);
		postFeature.code = "// post-feature";
	}

	void addFeatureCode(const FeatureShaderCode &code) {
		std::string uniqueId = code.name.empty() ? fmt::format("feature_{}", counter) : fmt::format("feature_{}_{}", code.name, counter);
		counter += 1;

		const char *wrapFeatureCodeFormat = "#define main {}\n{}\n#undef main";
		std::string wrappedCode = fmt::format(wrapFeatureCodeFormat, uniqueId, code.code);
		featureCodeSegments.emplace_back(code.name, wrappedCode, code.dependencies);

		const char *featureEntryCodeFormat = "{}(mi);";
		CodeBlock &mainBlock = mainCodeSegments.emplace_back(code.name, fmt::format(featureEntryCodeFormat, uniqueId));
		mainBlock.dependencies.push_back(NamedDependency(l_preFeature, DependencyType::After));
		mainBlock.dependencies.push_back(NamedDependency(l_postFeature, DependencyType::Before));
		for (auto dep : code.dependencies) {
			mainBlock.dependencies.push_back(dep);
		}
	}
};

struct ShaderBuilder {
	const std::vector<MeshVertexAttribute> &vertexAttributes;
	const std::vector<PipelineOutputDesc> &outputs;
	ShaderStageBuilder vertex;
	ShaderStageBuilder fragment;
	std::unordered_map<std::string, std::string> defines;
	std::vector<FeatureShaderField> globals;
	std::vector<FeatureShaderField> varyings;

	std::vector<std::string> errors;
	std::set<bgfx::Attrib::Enum> allocatedVaryings;

	void pushError(std::string &&err) { errors.emplace_back(err); }
	bool build(const FeatureBindingLayout &bindingLayout, ShaderCompilerInputs &output);
	bool buildVaryings(std::string &out);
	bool buildInputAttributes(std::string &out);
	bool buildUniformDefinition(const FeatureBindingLayout &bindingLayout, std::string &out);
	bool buildVertexOutputs();
	bool buildFragmentOutputs();
	bool buildSubShaderBody(ShaderStageBuilder &stageBuilder, std::string &output, const char *stageName);
};

bool ShaderBuilder::buildVaryings(std::string &out) {
	for (auto &field : varyings) {
		switch (field.type) {
		case FieldType::Float:
		case FieldType::Float2:
		case FieldType::Float3:
		case FieldType::Float4:
			break;
		default:
			pushError(fmt::format("Varying field {} has invalid type {}", field.name, magic_enum::enum_name(field.type)));
			return false;
		}

		bgfx::Attrib::Enum tag = allocateAttribTag(allocatedVaryings, field.name);
		if (tag == bgfx::Attrib::Count) {
			pushError(fmt::format("Not enough slots to allocate varying {}", field.name));
			return false;
		}

		std::string glslTypeName = getBasicFieldGLSLName(field.type);
		std::string varyingName = std::string("v_") + field.name;
		out += fmt::format("{} {} : {} = ", glslTypeName, varyingName, bgfxAttribToShaderSemantic(tag));
		std::string initializer;
		getBasicFieldGLSLValueOrDefault(field.type, field.defaultValue, initializer);
		out += initializer;
		out += ";\n";

		vertex.outputs.emplace_back(varyingName, glslTypeName, field.name, initializer);
		fragment.inputs.emplace_back(varyingName, glslTypeName, field.name);
	}

	return true;
}

bool ShaderBuilder::buildInputAttributes(std::string &out) {
	for (size_t i = 0; i < vertexAttributes.size(); i++) {
		auto &attr = vertexAttributes[i];

		std::string tagName = std::string(magic_enum::enum_name(attr.tag));
		boost::algorithm::to_lower(tagName);

		FieldType fieldType;
		if (attr.type != bgfx::AttribType::Float) {
			pushError(fmt::format("Vertex attribute {} should be float", tagName, attr.numComponents));
			return false;
		}

		std::string key = fmt::format("VERTEX_{}_COMPONENTS", tagName);
		boost::algorithm::to_upper(key);
		defines.insert_or_assign(key, fmt::format("{}", attr.numComponents));

		switch (attr.tag) {
		case bgfx::Attrib::Position:
		case bgfx::Attrib::Normal:
		case bgfx::Attrib::Tangent:
		case bgfx::Attrib::Bitangent:
			if (attr.numComponents != 3 && attr.type != bgfx::AttribType::Float) {
				pushError(fmt::format("Vertex attribute {} expects 3 components", tagName, attr.numComponents));
				return false;
			}
			fieldType = FieldType::Float3;
			break;
		default:
			switch (attr.numComponents) {
			case 1:
				fieldType = FieldType::Float;
				break;
			case 2:
				fieldType = FieldType::Float2;
				break;
			case 3:
				fieldType = FieldType::Float3;
				break;
			case 4:
				fieldType = FieldType::Float4;
				break;
			default:
				pushError(fmt::format("Invalid number of components ({}) for vertex attribute {}", attr.numComponents, tagName));
				return false;
			}
			break;
		}

		std::string attributeName = std::string("a_") + tagName;
		std::string glslTypeName = getBasicFieldGLSLName(fieldType);
		out += fmt::format("{} {} : {};\n", glslTypeName, attributeName, bgfxAttribToShaderSemantic(attr.tag));

		vertex.inputs.emplace_back(attributeName, glslTypeName, fmt::format("in_{}", tagName));
	}

	return true;
}

bool ShaderBuilder::buildUniformDefinition(const FeatureBindingLayout &bindingLayout, std::string &out) {
	for (auto &binding : bindingLayout.basicBindings) {
		out += fmt::format("uniform {} u_{};\n", getBasicFieldGLSLName(binding.type), binding.name);
	}

	for (size_t i = 0; i < bindingLayout.textureBindings.size(); i++) {
		auto &binding = bindingLayout.textureBindings[i];
		out += fmt::format("SAMPLER2D(u_{}, {});\n", binding.name, i);
	}

	return true;
}

bool ShaderBuilder::buildVertexOutputs() {
	CodeBlock &codeBlock = vertex.mainCodeSegments.emplace_back();
	codeBlock.name = l_writeOutputs;
	codeBlock.dependencies.emplace_back(l_postFeature, DependencyType::After);

	std::vector<InOutVar> outputs = {InOutVar("gl_Position", "vec4", "position")};

	for (InOutVar &output : outputs) {
		vertex.materialStructBody += fmt::format("vec4 out_{};\n", output.baseName);
		vertex.materialStructInitializer += fmt::format("mi.out_{} = vec4_splat(0.0);\n", output.baseName);

		codeBlock.code += fmt::format("{} = mi.out_{};\n", output.name, output.baseName);
	}

	return true;
}

bool ShaderBuilder::buildFragmentOutputs() {
	CodeBlock &codeBlock = fragment.mainCodeSegments.emplace_back();
	codeBlock.name = l_writeOutputs;
	codeBlock.dependencies.emplace_back(l_postFeature, DependencyType::After);

	if (outputs.size() >= 1) {
		std::string colorFieldName = outputs.size() > 0 ? outputs[0].name : "color";

		std::string key = fmt::format("OUTPUT_{}", colorFieldName);
		defines.insert_or_assign(key, fmt::format("{}", 0));

		fragment.materialStructBody += fmt::format("vec4 out_{};\n", colorFieldName);
		fragment.materialStructInitializer += fmt::format("mi.out_{} = vec4_splat(0.0);\n", colorFieldName);

		codeBlock.code = fmt::format("gl_FragColor = mi.out_{};\n", colorFieldName);
	} else {
		for (size_t i = 0; i < outputs.size(); i++) {
			auto &output = outputs[i];

			std::string key = fmt::format("OUTPUT_{}", output.name);
			defines.insert_or_assign(key, fmt::format("{}", i));

			fragment.materialStructBody += fmt::format("vec4 out_{};\n", output.name);
			fragment.materialStructInitializer += fmt::format("mi.out_{} = vec4_splat(0.0);\n", output.name);

			codeBlock.code += fmt::format("gl_FragData[{}] = mi.out_{};\n", i, output.name);
		}
	}

	return true;
}

bool ShaderBuilder::buildSubShaderBody(ShaderStageBuilder &stageBuilder, std::string &output, const char *stageName) {
	std::string materialStructInitializer = std::move(stageBuilder.materialStructInitializer);
	std::string writeOutputs;
	std::string materialStruct = "struct MaterialInfo {\n";
	materialStruct += stageBuilder.materialStructBody;
	for (size_t i = 0; i < globals.size(); i++) {
		auto &field = globals[i];
		if (!isBasicFieldType(field.type)) {
			pushError(fmt::format("Global field {} has invalid type {}", field.name, magic_enum::enum_name(field.type)));
			return false;
		}

		materialStruct += fmt::format("{} {};\n", getBasicFieldGLSLName(field.type), field.name);

		materialStructInitializer += "mi.{} = ";
		getBasicFieldGLSLValueOrDefault(field.type, field.defaultValue, materialStructInitializer);
		materialStructInitializer += ";\n";
	}

	for (auto &input : stageBuilder.inputs) {
		materialStruct += fmt::format("{} {};\n", input.type, input.baseName);
		materialStructInitializer += fmt::format("mi.{} = {};\n", input.baseName, input.name);
	}

	for (auto &output : stageBuilder.outputs) {
		materialStruct += fmt::format("{} {};\n", output.type, output.baseName);
		if (!output.initializer.empty())
			materialStructInitializer += fmt::format("mi.{} = {};\n", output.baseName, output.initializer);
		writeOutputs += fmt::format("{} = mi.{};\n", output.name, output.baseName);
	}

	materialStruct += "};";

	output.clear();
	output += materialStruct + "\n";

	if (!resolveCodeBlocks(stageBuilder.featureCodeSegments, output)) {
		pushError(fmt::format("Can not resolve feature code blocks for stage {}", stageName));
		return false;
	}

	output += "\nvoid main() {\n"
			  "MaterialInfo mi;\n";
	output += materialStructInitializer;
	if (!resolveCodeBlocks(stageBuilder.mainCodeSegments, output)) {
		pushError(fmt::format("Can not resolve feature code blocks for stage {}", stageName));
		return false;
	}

	output += "\n" + writeOutputs;
	output += "}\n";

	return true;
}

bool ShaderBuilder::build(const FeatureBindingLayout &bindingLayout, ShaderCompilerInputs &output) {
	output.varyings.clear();
	if (!buildVaryings(output.varyings))
		return false;

	if (!buildInputAttributes(output.varyings))
		return false;

	if (!buildVertexOutputs())
		return false;

	if (!buildFragmentOutputs())
		return false;

	std::string vertexCode;
	if (!buildSubShaderBody(vertex, vertexCode, "vertex"))
		return false;

	std::string fragmentCode;
	if (!buildSubShaderBody(fragment, fragmentCode, "fragment"))
		return false;

	std::string vsHeader, fsHeader, sharedHeader;
	auto appendInOutVars = [](const std::vector<InOutVar> &vars, std::string &output) {
		for (size_t i = 0; i < vars.size(); i++) {
			output += vars[i].name;
			if ((i + 1) < vars.size())
				output += ", ";
		}
	};
	vsHeader += "$input ";
	appendInOutVars(vertex.inputs, vsHeader);
	vsHeader += "\n";

	vsHeader += "$output ";
	appendInOutVars(vertex.outputs, vsHeader);
	vsHeader += "\n";

	fsHeader += "$input ";
	appendInOutVars(fragment.inputs, fsHeader);
	fsHeader += "\n";

	sharedHeader += "\n";
	if (!buildUniformDefinition(bindingLayout, sharedHeader))
		return false;
	sharedHeader += "\n";

	for (auto &define : defines) {
		sharedHeader += fmt::format("#define {} {}\n", define.first, define.second);
	}
	sharedHeader += "#include <bgfx_shader.sh>\n";

	output.stages[0].code = vsHeader + sharedHeader + vertexCode;
	output.stages[1].code = fsHeader + sharedHeader + fragmentCode;

	return true;
}

FeaturePipeline::~FeaturePipeline() {
	if (bgfx::isValid(program))
		bgfx::destroy(program);
}

FeatureBindingLayout::~FeatureBindingLayout() {
	for (auto &binding : basicBindings)
		bgfx::destroy(binding.handle);
	for (auto &binding : textureBindings)
		bgfx::destroy(binding.handle);
}

bool buildBindingLayout(const BuildBindingLayoutParams &params, FeatureBindingLayout &outBindingLayout) {
	auto &material = params.drawable.material;

	auto applyFeature = [&](auto &featurePtr) {
		for (auto &shaderParam : featurePtr->shaderParams) {
			std::string shaderName = fmt::format("u_{}", shaderParam.name);
			if (shaderParam.type == FieldType::Texture2D) {
				auto &binding = outBindingLayout.textureBindings.emplace_back();
				binding.name = shaderParam.name;
				binding.handle = bgfx::createUniform(shaderName.c_str(), bgfx::UniformType::Sampler);
				binding.type = FieldType::Texture2D;
				if (const TexturePtr *texture = std::get_if<TexturePtr>(&shaderParam.defaultValue)) {
					binding.defaultValue = *texture;
				}
			} else {
				auto &binding = outBindingLayout.basicBindings.emplace_back();
				binding.name = shaderParam.name;
				binding.defaultValue = shaderParam.defaultValue;
				binding.type = shaderParam.type;
				bgfx::UniformType::Enum uniformType;
				switch (shaderParam.type) {
				case FieldType::Float:
				case FieldType::Float2:
				case FieldType::Float3:
				case FieldType::Float4:
					uniformType = bgfx::UniformType::Vec4;
					break;
				case FieldType::Float4x4:
					uniformType = bgfx::UniformType::Mat4;
					break;
				default:
					uniformType = bgfx::UniformType::Count;
					assert(false);
					break;
				}
				binding.handle = bgfx::createUniform(shaderName.c_str(), uniformType);
			}
		}
	};

	for (auto &featurePtr : params.features) {
		applyFeature(featurePtr);
	}
	if (material) {
		for (auto &featurePtr : material->customFeatures) {
			applyFeature(featurePtr);
		}
	}

	return true;
}

bool buildPipeline(const BuildPipelineParams &params, FeaturePipeline &outPipeline, ShaderCompilerInputs *outShaderCompilerInputs) {
	ShaderBuilder shaderBuilder{params.vertexAttributes, params.outputs};
	FeaturePipelineState combinedState;

	auto applyFeature = [&](auto &featurePtr) {
		const Feature &feature = *featurePtr;
		combinedState = combinedState.combine(feature.state);

		for (auto &shaderCode : feature.shaderCode) {
			if (shaderCode.stage == ProgrammableGraphicsStage::Vertex)
				shaderBuilder.vertex.addFeatureCode(shaderCode);
			else
				shaderBuilder.fragment.addFeatureCode(shaderCode);
		}

		for (auto &varying : feature.shaderVaryings) {
			shaderBuilder.varyings.push_back(varying);
		}
	};

	for (auto &featurePtr : params.features) {
		applyFeature(featurePtr);
	}

	ShaderCompilerInputs localShaderCompilerInputs;
	if (!outShaderCompilerInputs)
		outShaderCompilerInputs = &localShaderCompilerInputs;
	ShaderCompilerInputs &shaderCompilerInputs = *outShaderCompilerInputs;

	if (!shaderBuilder.build(params.bindingLayout, shaderCompilerInputs))
		return false;

	if (params.rendererType != RendererType::None) {
		ShaderCompileOptions compileOptions;
		ShaderCompileOutput vsOutput, fsOutput;

		compileOptions.keepOutputs = true;
		compileOptions.debugInformation = true;

		assert(params.shaderCompiler);
		compileOptions.shaderType = 'v';
		compileOptions.setCompatibleForRendererType(params.rendererType);
		if (!params.shaderCompiler->compile(compileOptions, vsOutput, shaderCompilerInputs.varyings.c_str(), shaderCompilerInputs.stages[0].code.c_str(),
											shaderCompilerInputs.stages[0].code.size()))
			return false;

		compileOptions.shaderType = 'f';
		compileOptions.setCompatibleForRendererType(params.rendererType);
		if (!params.shaderCompiler->compile(compileOptions, fsOutput, shaderCompilerInputs.varyings.c_str(), shaderCompilerInputs.stages[1].code.c_str(),
											shaderCompilerInputs.stages[1].code.size()))
			return false;

		if (params.rendererType == RendererType::OpenGL) {
			spdlog::info("-- Platform VS:\n{}", extractPlatformShaderBinary(vsOutput.binary));
			spdlog::info("-- Platform FS:\n{}", extractPlatformShaderBinary(fsOutput.binary));
		}

		bgfx::ShaderHandle vs = bgfx::createShader(bgfx::copy(vsOutput.binary.data(), vsOutput.binary.size()));
		bgfx::ShaderHandle fs = bgfx::createShader(bgfx::copy(fsOutput.binary.data(), fsOutput.binary.size()));
		outPipeline.program = bgfx::createProgram(vs, fs, true);
	}

	spdlog::info("-- Varyings:\n{}", shaderCompilerInputs.varyings);

	outPipeline.state = combinedState.toBGFXState(params.faceWindingOrder);

	return true;
}

} // namespace gfx
