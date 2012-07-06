#!/usr/bin/env python

def options(opt):
    opt.tool_options("compiler_cxx")

def configure(conf):
    conf.check_tool("compiler_cxx")

def build(bld):
    bld(features='c cxx cxxprogram', target='main', source="multitasking.cc main.cc")
