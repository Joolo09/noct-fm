# SPDX-License-Identifier: MPL-2.0
from .noct import Noct


def create_instance(c_instance):
    return Noct(c_instance)
