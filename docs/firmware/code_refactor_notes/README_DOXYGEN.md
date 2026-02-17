# Doxygen for firmware

Generate cross-referenced documentation (variables, functions, file:line) from the firmware C sources.

**From the `firmware/` directory:**

```bash
doxygen docs/Doxyfile
```

Then open `firmware/docs/html/index.html` in a browser. Use “Files” → select a `.c`/`.h` → “Variable Documentation” (or “Data Fields”) to see where each symbol is defined and “Referenced by” for usage sites (file:line).

The Doxyfile excludes `firmware/test/` so only the main AVR app and protocol are documented.
