/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2021 Fragcolor Pte. Ltd. */

// Pass the *.js file to run as argument
const test = require(process.argv[2]);
test({
  postRun: () => {
    process.kill(process.pid, "SIGTERM");
  }
});
