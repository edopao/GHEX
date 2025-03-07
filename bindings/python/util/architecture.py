#
# ghex-org
#
# Copyright (c) 2014-2023, ETH Zurich
# All rights reserved.
#
# Please, refer to the LICENSE file in the root directory.
# SPDX-License-Identifier: BSD-3-Clause
#
from enum import Enum


class architecture(Enum):
    CPU = "ghex::cpu"
    GPU = "ghex::gpu"
