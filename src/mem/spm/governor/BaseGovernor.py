# Authors: Donny, Majid


from m5.params import *
from m5.SimObject import SimObject

class BaseGovernor(SimObject):
    type = 'BaseGovernor'
    cxx_header = "mem/spm/governor/base.hh"
    gov_type = Param.String("Local", "Governor type") 
    col = Param.Int("number of columns")
    row = Param.Int("number of rows")
    local_share = Param.Float(0.75, "reserved share of local spm")
    guest_slot_selection_policy = Param.String("LeastRecentlyUsed", "Guest Slot Selection Policy")
    guest_slot_relocation_policy = Param.String("MoveOffChip", "Guest Slot Relocation Policy")
    hybrid_mem = Param.Bool(False, "hybrid memory (SPM/Cache)?")
    uncacheable_spm = Param.Bool(False, "If unallocated SPM pages should be uncacheable")

class LocalSPM(BaseGovernor):
    type = 'LocalSPM'
    cxx_class = 'LocalSPM'
    cxx_header = "mem/spm/governor/local_spm.hh"
    
class RandomSPM(BaseGovernor):
    type = 'RandomSPM'
    cxx_class = 'RandomSPM'
    cxx_header = "mem/spm/governor/random_spm.hh"
    
class GreedySPM(BaseGovernor):
    type = 'GreedySPM'
    cxx_class = 'GreedySPM'
    cxx_header = "mem/spm/governor/greedy_spm.hh"
    
class GuaranteedGreedySPM(BaseGovernor):
    type = 'GuaranteedGreedySPM'
    cxx_class = 'GuaranteedGreedySPM'
    cxx_header = "mem/spm/governor/guaranteed_greedy_spm.hh"