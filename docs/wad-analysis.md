# WAD Analysis: The_Spiky_Channel.wad

Source file:
`/Users/gianschreiner/Downloads/The_Spiky_Channel.wad`

SHA-256:
`31cc8956604b13e5269f365c834cf1fd146332d5c6eace3e7cad3af6206b0726`

This analysis is read-only. The WAD was not installed or modified.

## Container

The file uses a standard installable Wii WAD layout:

```text
Header            0x00000000 size 0x20
Certificates      0x00000040 size 0x0a00
Ticket            0x00000a40 size 0x02a4
TMD               0x00000d00 size 0x022c
Content data      0x00000f40 size 0x1bba40
Footer            0x001bc980 size 0x40
```

File size: `0x1bc9c0` bytes, 1.7 MiB.

WAD type field: `Is`.

## Ticket

- Issuer: `Root-CA00000001-XS00000003`
- Title ID: `00010001-52484243`
- Low Title ID as ASCII: `RHBC`
- Title key field bytes decode as ASCII text: `GottaGetSomeBeer`

## TMD

- Issuer: `Root-CA00000001-CP00000004`
- System version: `00000001-0000003a`
- Title ID: `00010001-52484243`
- Low Title ID as ASCII: `RHBC`
- Group ID: `0x4842`
- Access rights: `0x00000003`
- Title version: `258`
- Content count: `2`
- Boot index: `1`

## Contents

```text
Content 0
  ID:    0x00000000
  Index: 0
  Type:  0x0001
  Size:  668648 bytes
  SHA1:  c1a5f465823b9040a57c6c3f5153a8d7d88f973b

Content 1
  ID:    0x00000001
  Index: 1
  Type:  0x0001
  Size:  1148448 bytes
  SHA1:  ccfebb7e713a4cc63e421cdbb907cb0c9598386e
```

Footer text:

```text
TmStmp1725783558
```

## Reuse Assessment

Potentially reusable later, after proper extraction and license/ownership
review:

- Banner layout and visual direction
- Icon visual direction
- Sound direction
- Channel/content structure as a reference point
- IMET/title metadata patterns after decrypted asset inspection

Not suitable as-is for the new public Spiky forwarder:

- The Title ID `00010001-52484243` / `RHBC` should not be reused.
- The internal string `GottaGetSomeBeer` is not appropriate for the final
  public project metadata.
- The WAD should not be repacked directly into the new channel.

Recommended later:

- Choose a unique non-system Title ID for Spiky.
- Extract banner/icon/sound only with proper WAD/U8 tooling in a read-only
  workspace.
- Keep the final forwarder minimal and load only `usb:/spiky/core/boot.dol`.
- Validate generated WAD structure in Dolphin before any hardware testing.

