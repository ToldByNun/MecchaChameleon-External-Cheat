## U1

---

### FNames
**Function:** `unk_149E4B800`

**Pattern:**
`48 8D 05 ?? ?? ?? ?? 48 8B D1 48 8B C8`

**Match:**
`PenguinHotel-Win64-Shipping.exe+none`

**Notes:**
- Fuck the devs for pushing an update after me trying to test an AOB
- ^

**Additionals**
Match Count: `0`  
Resolve: `-`  
Verified: `-`

---

### GWorld
**Function:** `qword_14A0BD770`

**Pattern:**
`-`

**Match:**
`PenguinHotel-Win64-Shipping.exe+-`

**Notes:**
- None

**Additionals**
Match Count: `-`  
Resolve: `-`  
Verified: `-`

---

## U2

---

### FNames
**Function:** `unk_149E40280`

**Pattern:**
`80 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? E9 ?? ?? ?? ??`

**Instruction:**
`48 8D 05 ?? ?? ?? ??` / `lea rax, [rip+rel32]`

**Match:**
`PenguinHotel-Win64-Shipping.exe+13AE2E7`

**Notes:**
- First working AOB using the ida_wildcard.py tool
- Waiting for next update to test further

**Additionals**
Match Count: `1`  
Resolve: `AOB + 0xD, 3, 7`  
Verified: `06/23/2026 11:01`

---