# kameo-nx (2026-11-03): the pixel shader of one draw as the driver was given it (for Ryujinx: its translation of the Maxwell
# code, which shows what uam compiled). Run inside qrenderdoc, through tools/rdc_run.ps1:
#   powershell -File tools/rdc_run.ps1 -Script rdc_shader.py -In X.rdc -Out dir -First EVENT -Last EVENT
# Writes shader.txt: the disassembly targets, then the first target's text.
import os
import traceback

import renderdoc as rd

IN = os.environ.get("RDC_IN")
OUT = os.environ.get("RDC_OUT")
FIRST = int(os.environ.get("RDC_FIRST", "0"))
log = open(os.path.join(OUT, "shader.txt"), "w")

try:
    cap = rd.OpenCaptureFile()
    if cap.OpenFile(IN, "", None) != rd.ResultCode.Succeeded:
        raise RuntimeError("open file")
    result = cap.OpenCapture(rd.ReplayOptions(), None)
    status, controller = result if isinstance(result, tuple) else (rd.ResultCode.Succeeded, result)
    if status != rd.ResultCode.Succeeded:
        raise RuntimeError("open capture: %s" % status)
    controller.SetFrameEvent(FIRST, True)
    st = controller.GetPipelineState()
    refl = st.GetShaderReflection(rd.ShaderStage.Pixel)
    targets = controller.GetDisassemblyTargets(True)
    log.write("targets: %s\n" % ", ".join(targets))
    pipe = st.GetGraphicsPipelineObject()
    text = controller.DisassembleShader(pipe, refl, targets[0])
    log.write(text)
    controller.Shutdown()
    cap.Shutdown()
except Exception:
    log.write("ERROR " + traceback.format_exc())
log.close()
os._exit(0)
