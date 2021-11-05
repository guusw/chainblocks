static const uint8_t fs_picking_glsl[116] =
{
	0x46, 0x53, 0x48, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0c, 0x75, // FSH............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x02, 0x01, 0x00, 0x00, 0x01, // _picking_id.....
	0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x20, 0x76, // ...L...uniform v
	0x65, 0x63, 0x34, 0x20, 0x75, 0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, // ec4 u_picking_id
	0x3b, 0x0a, 0x76, 0x6f, 0x69, 0x64, 0x20, 0x6d, 0x61, 0x69, 0x6e, 0x20, 0x28, 0x29, 0x0a, 0x7b, // ;.void main ().{
	0x0a, 0x20, 0x20, 0x67, 0x6c, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x20, // .  gl_FragColor 
	0x3d, 0x20, 0x75, 0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x3b, 0x0a, // = u_picking_id;.
	0x7d, 0x0a, 0x0a, 0x00,                                                                         // }...
};
static const uint8_t fs_picking_essl[122] =
{
	0x46, 0x53, 0x48, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0c, 0x75, // FSH............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x02, 0x01, 0x00, 0x00, 0x01, // _picking_id.....
	0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x20, 0x68, // ...R...uniform h
	0x69, 0x67, 0x68, 0x70, 0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x75, 0x5f, 0x70, 0x69, 0x63, 0x6b, // ighp vec4 u_pick
	0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x3b, 0x0a, 0x76, 0x6f, 0x69, 0x64, 0x20, 0x6d, 0x61, 0x69, // ing_id;.void mai
	0x6e, 0x20, 0x28, 0x29, 0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x67, 0x6c, 0x5f, 0x46, 0x72, 0x61, 0x67, // n ().{.  gl_Frag
	0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x3d, 0x20, 0x75, 0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, // Color = u_pickin
	0x67, 0x5f, 0x69, 0x64, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x00,                                     // g_id;.}...
};
static const uint8_t fs_picking_spv[671] =
{
	0x46, 0x53, 0x48, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0c, 0x75, // FSH............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x12, 0x01, 0x00, 0x00, 0x01, // _picking_id.....
	0x00, 0x00, 0x00, 0x74, 0x02, 0x00, 0x00, 0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0a, // ...t.....#......
	0x00, 0x08, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, // ...L............
	0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, // ...........GLSL.
	0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, // std.450.........
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, // ................
	0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x31, // ...main....(...1
	0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, // ................
	0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0xf4, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, // ................
	0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x1e, // ...main.........
	0x00, 0x00, 0x00, 0x55, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x00, // ...UniformBlock.
	0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, // ...............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x00, 0x00, 0x00, 0x00, 0x05, // _picking_id.....
	0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x28, // ... ...........(
	0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x43, 0x6f, 0x6f, 0x72, 0x64, 0x00, // ...gl_FragCoord.
	0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x31, 0x00, 0x00, 0x00, 0x62, 0x67, 0x66, 0x78, 0x5f, // .......1...bgfx_
	0x46, 0x72, 0x61, 0x67, 0x44, 0x61, 0x74, 0x61, 0x30, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x1e, // FragData0..H....
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, // .......#.......G
	0x00, 0x03, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x20, // ...........G... 
	0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x20, // ...".......G... 
	0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x28, // ...!...0...G...(
	0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x31, // ...........G...1
	0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, // ................
	0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, // ...!............
	0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x08, // ....... ........
	0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x03, 0x00, 0x1e, // ................
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x02, // ....... ........
	0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x20, // .......;....... 
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x21, 0x00, 0x00, 0x00, 0x20, // ...........!... 
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x21, 0x00, 0x00, 0x00, 0x22, // .......+...!..."
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x02, // ....... ...#....
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x27, 0x00, 0x00, 0x00, 0x01, // ....... ...'....
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x27, 0x00, 0x00, 0x00, 0x28, // .......;...'...(
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x30, 0x00, 0x00, 0x00, 0x03, // ....... ...0....
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x30, 0x00, 0x00, 0x00, 0x31, // .......;...0...1
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, // .......6........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, // ................
	0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x23, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x20, // ...A...#...J... 
	0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4b, // ..."...=.......K
	0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x31, 0x00, 0x00, 0x00, 0x4b, // ...J...>...1...K
	0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00,       // .......8.......
};
static const uint8_t fs_picking_dx9[196] =
{
	0x46, 0x53, 0x48, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0c, 0x75, // FSH............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x12, 0x01, 0x00, 0x00, 0x01, // _picking_id.....
	0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xfe, 0xff, 0x21, 0x00, 0x43, // .............!.C
	0x54, 0x41, 0x42, 0x1c, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0x01, // TAB....W........
	0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x30, // ...........P...0
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, // ...........@....
	0x00, 0x00, 0x00, 0x75, 0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x00, // ...u_picking_id.
	0xab, 0xab, 0xab, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, // ................
	0x00, 0x00, 0x00, 0x70, 0x73, 0x5f, 0x33, 0x5f, 0x30, 0x00, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, // ...ps_3_0.Micros
	0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, // oft (R) HLSL Sha
	0x64, 0x65, 0x72, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2e, // der Compiler 10.
	0x31, 0x00, 0xab, 0x01, 0x00, 0x00, 0x02, 0x00, 0x08, 0x0f, 0x80, 0x00, 0x00, 0xe4, 0xa0, 0xff, // 1...............
	0xff, 0x00, 0x00, 0x00,                                                                         // ....
};
static const uint8_t fs_picking_dx11[263] =
{
	0x46, 0x53, 0x48, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0c, 0x75, // FSH............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x12, 0x00, 0x00, 0x00, 0x01, // _picking_id.....
	0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x44, 0x58, 0x42, 0x43, 0x97, 0x89, 0xd6, 0x18, 0xd7, // .......DXBC.....
	0x24, 0x4d, 0xea, 0xd1, 0xcc, 0xac, 0xc3, 0xb2, 0xf1, 0x52, 0x06, 0x01, 0x00, 0x00, 0x00, 0xdc, // $M.......R......
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x94, // .......,...`....
	0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x4e, 0x2c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, // ...ISGN,........
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, // ... ............
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5f, 0x50, 0x4f, // ...........SV_PO
	0x53, 0x49, 0x54, 0x49, 0x4f, 0x4e, 0x00, 0x4f, 0x53, 0x47, 0x4e, 0x2c, 0x00, 0x00, 0x00, 0x01, // SITION.OSGN,....
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ....... ........
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x53, // ...............S
	0x56, 0x5f, 0x54, 0x41, 0x52, 0x47, 0x45, 0x54, 0x00, 0xab, 0xab, 0x53, 0x48, 0x44, 0x52, 0x40, // V_TARGET...SHDR@
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x04, 0x46, // ...@.......Y...F
	0x8e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xf2, // . .........e....
	0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x06, 0xf2, 0x20, 0x10, 0x00, 0x00, //  ......6.... ...
	0x00, 0x00, 0x00, 0x46, 0x8e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, // ...F. .........>
	0x00, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00,                                                       // .......
};
static const uint8_t fs_picking_mtl[404] =
{
	0x46, 0x53, 0x48, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0c, 0x75, // FSH............u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x12, 0x01, 0x00, 0x00, 0x01, // _picking_id.....
	0x00, 0x00, 0x00, 0x69, 0x01, 0x00, 0x00, 0x23, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, // ...i...#include 
	0x3c, 0x6d, 0x65, 0x74, 0x61, 0x6c, 0x5f, 0x73, 0x74, 0x64, 0x6c, 0x69, 0x62, 0x3e, 0x0a, 0x23, // <metal_stdlib>.#
	0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x20, 0x3c, 0x73, 0x69, 0x6d, 0x64, 0x2f, 0x73, 0x69, // include <simd/si
	0x6d, 0x64, 0x2e, 0x68, 0x3e, 0x0a, 0x0a, 0x75, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x6e, 0x61, 0x6d, // md.h>..using nam
	0x65, 0x73, 0x70, 0x61, 0x63, 0x65, 0x20, 0x6d, 0x65, 0x74, 0x61, 0x6c, 0x3b, 0x0a, 0x0a, 0x73, // espace metal;..s
	0x74, 0x72, 0x75, 0x63, 0x74, 0x20, 0x5f, 0x47, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x0a, 0x7b, 0x0a, // truct _Global.{.
	0x20, 0x20, 0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x34, 0x20, 0x75, 0x5f, 0x70, 0x69, 0x63, //     float4 u_pic
	0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x3b, 0x0a, 0x7d, 0x3b, 0x0a, 0x0a, 0x73, 0x74, 0x72, // king_id;.};..str
	0x75, 0x63, 0x74, 0x20, 0x78, 0x6c, 0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x5f, // uct xlatMtlMain_
	0x6f, 0x75, 0x74, 0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x66, 0x6c, 0x6f, 0x61, 0x74, 0x34, // out.{.    float4
	0x20, 0x62, 0x67, 0x66, 0x78, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x44, 0x61, 0x74, 0x61, 0x30, 0x20, //  bgfx_FragData0 
	0x5b, 0x5b, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x28, 0x30, 0x29, 0x5d, 0x5d, 0x3b, 0x0a, 0x7d, 0x3b, // [[color(0)]];.};
	0x0a, 0x0a, 0x66, 0x72, 0x61, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x20, 0x78, 0x6c, 0x61, 0x74, 0x4d, // ..fragment xlatM
	0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x5f, 0x6f, 0x75, 0x74, 0x20, 0x78, 0x6c, 0x61, 0x74, 0x4d, // tlMain_out xlatM
	0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, 0x28, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x20, // tlMain(constant 
	0x5f, 0x47, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x26, 0x20, 0x5f, 0x6d, 0x74, 0x6c, 0x5f, 0x75, 0x20, // _Global& _mtl_u 
	0x5b, 0x5b, 0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x28, 0x30, 0x29, 0x5d, 0x5d, 0x29, 0x0a, 0x7b, // [[buffer(0)]]).{
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x78, 0x6c, 0x61, 0x74, 0x4d, 0x74, 0x6c, 0x4d, 0x61, 0x69, 0x6e, // .    xlatMtlMain
	0x5f, 0x6f, 0x75, 0x74, 0x20, 0x6f, 0x75, 0x74, 0x20, 0x3d, 0x20, 0x7b, 0x7d, 0x3b, 0x0a, 0x20, // _out out = {};. 
	0x20, 0x20, 0x20, 0x6f, 0x75, 0x74, 0x2e, 0x62, 0x67, 0x66, 0x78, 0x5f, 0x46, 0x72, 0x61, 0x67, //    out.bgfx_Frag
	0x44, 0x61, 0x74, 0x61, 0x30, 0x20, 0x3d, 0x20, 0x5f, 0x6d, 0x74, 0x6c, 0x5f, 0x75, 0x2e, 0x75, // Data0 = _mtl_u.u
	0x5f, 0x70, 0x69, 0x63, 0x6b, 0x69, 0x6e, 0x67, 0x5f, 0x69, 0x64, 0x3b, 0x0a, 0x20, 0x20, 0x20, // _picking_id;.   
	0x20, 0x72, 0x65, 0x74, 0x75, 0x72, 0x6e, 0x20, 0x6f, 0x75, 0x74, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, //  return out;.}..
	0x00, 0x00, 0x10, 0x00,                                                                         // ....
};
extern const uint8_t* fs_picking_pssl;
extern const uint32_t fs_picking_pssl_size;
