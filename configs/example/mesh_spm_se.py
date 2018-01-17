# Copyright (c) 2012-2013 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt

# Simple test script
#
# "m5 test.py"

import optparse
import sys
import os

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('../')

from ruby import Ruby

from common import Options
from common import Simulation
from common import SPMConfig
from common import MemConfig
from common.Caches import *
from common.cpu2000 import *

from topologies import *

from network import Network

m5.disableAllListeners()

def cl_create_topology(controllers, options):
    exec "import topologies.%s as Topo" % options.topology
    topology = eval("Topo.%s(controllers)" % options.topology)
    return topology

def create_system(options, system, pmmu_nodes):

    topology = cl_create_topology(pmmu_nodes, options)

    NumVirtualNetworks = 10 #@TODO: Move this

    # Create the network object
    (network, IntLinkClass, ExtLinkClass, RouterClass, InterfaceClass) = \
        Network.create_network(options, system.ruby)
    system.network = network

    network.number_of_virtual_networks = NumVirtualNetworks
    system.ruby.number_of_virtual_networks = network.number_of_virtual_networks

    # Create the network topology
    topology.makeTopology(
      options, network, IntLinkClass, ExtLinkClass, RouterClass)

    # Initialize network based on topology
    Network.init_network(options, network, InterfaceClass)

###############################################################

def add_all_options():
    parser = optparse.OptionParser()
    Options.addCommonOptions(parser)
    Options.addSEOptions(parser)

    Network.define_options(parser)

    # architectural configuration options
    parser.add_option("--hybrid-mem", action="store_true")

    # spm options
    parser.add_option("--spm-page-size", type="int", default="512",
                      help="spm page size in bytes")
    parser.add_option("--l1dspm-size", type="string", default="64kB")

    # governor options
    parser.add_option("--gov-type", type="string", default="Local")
    parser.add_option("--local-share", type="float", default="0.75")
    parser.add_option("--guest-slot-selection-policy", type="string", default="LeastRecentlyUsed")
    parser.add_option("--guest-slot-relocation-policy", type="string", default="MoveOffChip")
    parser.add_option("--uncacheable-spm", action="store_true")

    # approx options
    parser.add_option("--spm-read-ber", type="float", default="-1",
                      help="read ber for approximate spm reads")
    parser.add_option("--spm-write-ber", type="float", default="-1",
                      help="write ber for approximate spm writes")
    parser.add_option("--ber-energy-file", type="string", default="")

    return parser

###############################################################

def get_processes(options):
    """Interprets provided options and returns a list of processes"""

    multiprocesses = []
    inputs = []
    outputs = []
    errouts = []
    pargs = []

    workloads = options.cmd.split(';')
    if options.input != "":
        inputs = options.input.split(';')
    if options.output != "":
        outputs = options.output.split(';')
    if options.errout != "":
        errouts = options.errout.split(';')
    if options.options != "":
        pargs = options.options.split(';')

    idx = 0
    for wrkld in workloads:
        process = Process(pid = 100 + idx)
        process.executable = wrkld
        process.cwd = os.getcwd()

        if len(pargs) > idx:
            process.cmd = [wrkld] + pargs[idx].split()
        else:
            process.cmd = [wrkld]

        if len(inputs) > idx:
            process.input = inputs[idx]
        if len(outputs) > idx:
            process.output = outputs[idx]
        if len(errouts) > idx:
            process.errout = errouts[idx]

        multiprocesses.append(process)
        idx += 1

    if options.smt:
        assert(options.cpu_type == "detailed" or options.cpu_type == "inorder")
        return multiprocesses, idx
    else:
        return multiprocesses, 1

###############################################################

parser = add_all_options()

if '--ruby' in sys.argv:
    Ruby.define_options(parser)

(options, args) = parser.parse_args()

if options.hybrid_mem:
    if options.spm_page_size <= options.cacheline_size:
        print "Error: spm page size must be greater than cacheline \
size when hybrid memory hierarchy is used"
        sys.exit(1)

if args:
    print "Error: script doesn't take any positional arguments"
    sys.exit(1)

multiprocesses = []
numThreads = 1

if options.bench:
    apps = options.bench.split("-")
    if len(apps) != options.num_cpus:
        print "number of benchmarks not equal to set num_cpus!"
        sys.exit(1)

    for app in apps:
        try:
            if buildEnv['TARGET_ISA'] == 'alpha':
                exec("workload = %s('alpha', 'tru64', '%s')" % (
                        app, options.spec_input))
            elif buildEnv['TARGET_ISA'] == 'arm':
                exec("workload = %s('arm_%s', 'linux', '%s')" % (
                        app, options.arm_iset, options.spec_input))
            else:
                exec("workload = %s(buildEnv['TARGET_ISA', 'linux', '%s')" % (
                        app, options.spec_input))
            multiprocesses.append(workload.makeLiveProcess())
        except:
            print >>sys.stderr, "Unable to find workload for %s: %s" % (
                    buildEnv['TARGET_ISA'], app)
            sys.exit(1)
elif options.cmd:
    multiprocesses, numThreads = get_processes(options)
else:
    print >> sys.stderr, "No workload specified. Exiting!\n"
    sys.exit(1)


(CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
CPUClass.numThreads = numThreads

# Check -- do not allow SMT with multiple CPUs
if options.smt and options.num_cpus > 1:
    fatal("You cannot use SMT with multiple CPUs!")

np = options.num_cpus
system = System(cpu = [CPUClass(cpu_id=i) for i in xrange(np)],
                mem_mode = test_mem_mode,
                mem_ranges = [AddrRange(options.mem_size)],
                cache_line_size = options.cacheline_size)

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(clock =  options.sys_clock,
                                   voltage_domain = system.voltage_domain)

# Create a CPU voltage domain
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                       voltage_domain = system.cpu_voltage_domain)

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain

# Sanity check
if options.fastmem:
    if CPUClass != AtomicSimpleCPU:
        fatal("Fastmem can only be used with atomic CPU!")
    if (options.caches or options.l2cache):
        fatal("You cannot use fastmem in combination with caches!")

if options.simpoint_profile:
    if not options.fastmem:
        # Atomic CPU checked with fastmem option already
        fatal("SimPoint generation should be done with atomic cpu and fastmem")
    if np > 1:
        fatal("SimPoint generation not supported with more than one CPUs")

for i in xrange(np):
    if options.smt:
        system.cpu[i].workload = multiprocesses
    elif len(multiprocesses) == 1:
        system.cpu[i].workload = multiprocesses[0]
    else:
        system.cpu[i].workload = multiprocesses[i]

    if options.fastmem:
        system.cpu[i].fastmem = True

    if options.simpoint_profile:
        system.cpu[i].simpoint_profile = True
        system.cpu[i].simpoint_interval = options.simpoint_interval

    if options.checker:
        system.cpu[i].addCheckerCpu()

    system.cpu[i].createThreads()


MemClass = Simulation.setMemClass(options)
#system.membus = SPMSystemXBar()
system.membus = SystemXBar()
system.system_port = system.membus.slave
system.governor = BaseGovernor(gov_type = options.gov_type,
                               col = options.num_cpus/options.mesh_rows,
                               row = options.mesh_rows,
                               local_share = options.local_share,
                               guest_slot_selection_policy = options.guest_slot_selection_policy,
                               guest_slot_relocation_policy = options.guest_slot_relocation_policy,
                               hybrid_mem = options.hybrid_mem,
                               uncacheable_spm = options.uncacheable_spm)
pmmus,sys = SPMConfig.config_cache(options, system)
MemConfig.config_mem(options, system)
system.ruby = RubySystem(num_of_sequencers=0,
                         block_size_bytes = options.cacheline_size)
create_system(options, system, pmmus)

root = Root(full_system = False, system = system)
Simulation.run(options, root, system, FutureClass)
