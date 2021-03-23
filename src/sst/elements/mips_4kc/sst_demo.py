import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
op.add_option("-c", "--cacheSz", action="store", type="int", dest="cacheSz", default=4)
op.add_option("-e", "--execFile0", action="store", type="string", dest="execFile0", default="test/terrcor-sst-demo")
op.add_option("-g", "--execFile1", action="store", type="string", dest="execFile1", default="test/tmp-demo")
op.add_option("-f", "--faultFile", action="store", type="string", dest="faultFile", default="demoFaults.txt")
(options, args) = op.parse_args()

sst.setProgramOption("stopAtCycle","1s")

# Define the simulation components
comp_mips0 = sst.Component("MIPS4KC_0", "mips_4kc.MIPS4KC")
comp_mips0.addParams({
    "verbose" : 0,
    "demo_mode" : 1,
    "execFile" : options.execFile0,
    "clock" : "1GHz",
    "fault_file" : options.faultFile, 
    "stack_top"  : 0x80000000,
    "timeout" : 220000000,
    "proc_num" : 0
})

comp_l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
comp_l1cache0.addParams({
    "debug" : 1,
    "debug_level" : 11,
    "access_latency_cycles" : "1",
    "cache_frequency" : "4 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "verbose" : 1,
    "L1" : "1",
    "cache_size" : "%dKiB"%options.cacheSz
})

comp_bus = sst.Component("bus", "memHierarchy.Bus")
comp_bus.addParams({
      "bus_frequency" : "4 Ghz"
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
    "debug" : 1,
    "coherence_protocol" : "MSI",
    "debug_level" : 10,
    "backend.access_time" : "10 ps",
    "backing" : "malloc", 
    "clock" : "4GHz",
    "backend.mem_size" : "512MiB",
    "addr_range_start" : "0x0",
    "addr_range_end" : "0x80000000"
})
comp_UART = sst.Component("UART", "memHierarchy.UART")
comp_UART.addParams({
    "verbose" : 0,
    "coherence_protocol" : "MSI",
    "debug_level" : 10,
    "backend.access_time" : "10 ps",
    "backing" : "malloc", 
    "clock" : "4GHz",
    "backend.mem_size" : "1MiB",
    "addr_range_start" : "0x90000000",
    #"addr_range_end" :   "0x90001000"
})

# Define the 2nd set of simulation components (TMP)
comp_mips1 = sst.Component("MIPS4KC_1", "mips_4kc.MIPS4KC")
comp_mips1.addParams({
    "verbose" : 0,
    "demo_mode" : 1,
    "execFile" : options.execFile1,
    "clock" : "100 MHz",
    #"fault_file" : options.faultFile, 
    "stack_top"  : 0x80000000,
    "timeout" : 22000000,
    "proc_num" : 1
})

comp_l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
comp_l1cache1.addParams({
    "debug" : 1,
    "debug_level" : 11,
    "access_latency_cycles" : "1",
    "cache_frequency" : "4 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "verbose" : 1,
    "L1" : "1",
    "cache_size" : "%dKiB"%options.cacheSz
})

comp_bus1 = sst.Component("bus1", "memHierarchy.Bus")
comp_bus1.addParams({
      "bus_frequency" : "4 Ghz"
})

comp_memory1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memory1.addParams({
    "debug" : 1,
    "coherence_protocol" : "MSI",
    "debug_level" : 10,
    "backend.access_time" : "10 ps",
    "backing" : "malloc", 
    "clock" : "4GHz",
    "backend.mem_size" : "512MiB",
    "addr_range_start" : "0x0",
    "addr_range_end" : "0x80000000"
})
comp_UART1 = sst.Component("UART1", "memHierarchy.UART")
comp_UART1.addParams({
    "verbose" : 0,
    "coherence_protocol" : "MSI",
    "debug_level" : 10,
    "backend.access_time" : "10 ps",
    "backing" : "malloc", 
    "clock" : "4GHz",
    "backend.mem_size" : "1MiB",
    "addr_range_start" : "0x90000000",
    #"addr_range_end" :   "0x90001000"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.enableAllStatisticsForComponentType("memHierarchy.Cache")

# Define the simulation links
link_mips_cache0 = sst.Link("link_mips_mem_0")
link_mips_cache0.connect( (comp_mips0, "mem_link", "10ps"), (comp_l1cache0, "high_network_0", "10ps") )
link_l1_bus0 = sst.Link("link_l1_bus_0")
link_l1_bus0.connect( (comp_l1cache0, "low_network_0", "10ps"), (comp_bus, "high_network_0", "10ps"))

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_bus, "low_network_0", "10ps"), (comp_memory, "direct_link", "10ps") )

link_uart_bus_link = sst.Link("link_uart_bus_link")
link_uart_bus_link.connect( (comp_bus, "low_network_1", "10ps"), (comp_UART, "direct_link", "10ps") )

# Define the 2nd set of simulation links
link_mips_cache1 = sst.Link("link_mips_mem_1")
link_mips_cache1.connect( (comp_mips1, "mem_link", "10ps"), (comp_l1cache1, "high_network_0", "10ps") )
link_l1_bus1 = sst.Link("link_l1_bus_1")
link_l1_bus1.connect( (comp_l1cache1, "low_network_0", "10ps"), (comp_bus1, "high_network_0", "10ps"))

link_mem_bus_link1 = sst.Link("link_mem_bus_link1")
link_mem_bus_link1.connect( (comp_bus1, "low_network_0", "10ps"), (comp_memory1, "direct_link", "10ps") )

link_uart_bus_link1 = sst.Link("link_uart_bus_link1")
link_uart_bus_link1.connect( (comp_bus1, "low_network_1", "10ps"), (comp_UART1, "direct_link", "10ps") )

# Connect the two sets via UART
link_uart_uart_link = sst.Link("link_uart_uart_link")
link_uart_uart_link.connect((comp_UART ,"other_UART","10ps"),
                            (comp_UART1,"other_UART","10ps"))

# Link RFP and TMP to allow for RFP reset
link_tmp_rfp = sst.Link("link_tmp_rfp")
link_tmp_rfp.connect((comp_mips0,"reset_link","10ps"),
                     (comp_mips1,"reset_link","10ps"))
