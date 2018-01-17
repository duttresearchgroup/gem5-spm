from m5.params import *
from m5.SimObject import SimObject
from Controller import RubyController

class PMMU(RubyController):
    type = 'PMMU'
    cxx_class = 'PMMU'
    cxx_header = "mem/spm/pmmu.hh"
#    version = Param.Int("");
    page_size_bytes = Param.Int(512,"Size of a SPM page in bytes")
    ruby_system = Param.RubySystem(NULL, "")
    responseFromSPM = Param.MessageBuffer("");
    responseToSPM = Param.MessageBuffer("");
    requestFromSPM = Param.MessageBuffer("");
    requestToSPM = Param.MessageBuffer("");
    responseToNetwork = Param.MessageBuffer("");
    requestToNetwork = Param.MessageBuffer("");
    governor = Param.BaseGovernor("")
    gov_type = Param.String("Local", "Governor type")
    spm_s_side = SlavePort("Slave port where SPM pushes requests/responses")
    spm_m_side = MasterPort("Master port to send requests/responses to SPM")
#    system = Param.System(Parent.any, "System we belong to")
#    system = Param.System("System we belong to")
#    spm_memory = Param.SPM("")
#    cache_memory = Param.BaseCache("")
