# Progress log

All times AEST, 2026-11-02 / 03.

- 20:06 Start. Docs read; `kameo-nx` created from katamari-nx-public's files.
- 20:10 Disc extracted to `D:\kameo\game` (5.4 GB); update unpacked (`default.xexp`, nine string tables to `D:\kameo\update`).
- 20:15 Katamari's parts taken out, as PAC-MAN's first commit did.
- 20:20 First codegen on the patched image: RePowered's hints checked, one wrong (`0x8251F320`), dropped. Pointer and gap sweeps, label check, wide scan, jump tables, register diet audits: clean.
- 20:25 First PC build started (emulated GPU, Renut compiled out).
