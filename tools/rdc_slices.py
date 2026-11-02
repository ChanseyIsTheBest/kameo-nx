# kameo-nx (2026-11-03): each texture a draw reads, with its layers and mips, and every mip of every layer saved
# (rdc_draw_detail.py saves layer 0, mip 0 only). Run inside qrenderdoc, through tools/rdc_run.ps1:
#   powershell -File tools/rdc_run.ps1 -Script rdc_slices.py -In X.rdc -Out dir -First EVENT -Last EVENT
# Writes slices.txt, and slice_<w>x<h>_<resource>_<layer>_<mip>.png.
import os
import traceback

import renderdoc as rd

IN = os.environ.get("RDC_IN")
OUT = os.environ.get("RDC_OUT")
FIRST = int(os.environ.get("RDC_FIRST", "0"))
log = open(os.path.join(OUT, "slices.txt"), "w")


def say(*parts):
    log.write(" ".join(str(p) for p in parts) + "\n")
    log.flush()


try:
    cap = rd.OpenCaptureFile()
    if cap.OpenFile(IN, "", None) != rd.ResultCode.Succeeded:
        raise RuntimeError("open file")
    result = cap.OpenCapture(rd.ReplayOptions(), None)
    status, controller = result if isinstance(result, tuple) else (rd.ResultCode.Succeeded, result)
    if status != rd.ResultCode.Succeeded:
        raise RuntimeError("open capture: %s" % status)
    textures = {t.resourceId: t for t in controller.GetTextures()}
    controller.SetFrameEvent(FIRST, True)
    st = controller.GetPipelineState()
    seen = set()
    for used in st.GetReadOnlyResources(rd.ShaderStage.Pixel):
        desc = used.descriptor if hasattr(used, "descriptor") else None
        rid = desc.resource if desc is not None else rd.ResourceId.Null()
        t = textures.get(rid)
        if not t or rid in seen:
            continue
        seen.add(rid)
        say("tex", int(rid), "%dx%d" % (t.width, t.height), t.format.Name(), "dim", t.dimension, "arraysize", t.arraysize, "mips", t.mips,
            "cube", t.cubemap, "view", desc.textureType, "first", desc.firstSlice, "slices", desc.numSlices, "swizzle", desc.swizzle.red,
            desc.swizzle.green, desc.swizzle.blue, desc.swizzle.alpha)
        if t.format.Name().startswith("D"):
            continue  # shadow maps: not uploaded, and their content follows the frame
        for layer in range(t.arraysize):
            for mip in range(t.mips):
                save = rd.TextureSave()
                save.resourceId = rid
                save.destType = rd.FileType.PNG
                save.alpha = rd.AlphaMapping.Preserve
                save.mip = mip
                save.slice.sliceIndex = layer
                path = os.path.join(OUT, "slice_%dx%d_%d_%d_%d.png" % (t.width, t.height, int(rid), layer, mip))
                ok = controller.SaveTexture(save, path) == rd.ResultCode.Succeeded
                if not ok:
                    say("  layer", layer, "mip", mip, "not saved")
    controller.Shutdown()
    cap.Shutdown()
    say("done")
except Exception:
    say("ERROR", traceback.format_exc())
log.close()
os._exit(0)
