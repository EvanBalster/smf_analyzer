# smf_analyzer

This tool scans a directory tree for MIDI files (.smf, .midi) and collects statistics (histograms) about MIDI events and Meta Events.

Currently these statistics concern message length and incidence:

- Incidence & Length of Non-Text Meta Events
- Incidence & Length of Text Meta Events
  - in bytes
  - in code points (corresponds closely to UTF-16 units)
  - in bytes, estimated, if it were re-encoded as UTF-8
- Incidence & Length of SysEx Events

## Command Line

Pre-compiled binaries are available in `bin` for Windows and Mac OS X.  The command line syntax is:

`smf_analyzer`[.exe] `<MIDI source folder>` `<output folder>`

Scanning is recursive through subfolders, and files with `.midi`, `.mid`, `.smf` and `.kar` files will be analyzed.  Histograms are in CSV format, describing all the files analyzed in one run.