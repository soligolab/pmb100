# PMB100 Terminal - Build AM3358

Questo progetto contiene il terminale PMB100 e una toolchain `make` per cross-compilare su **AM3358 (ARMv7, hard-float)**.

## Prerequisiti

- Toolchain ARMHF (default):
  - `~/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf`
- `make`
- Per build kernel 6: libreria `libgpiod` disponibile nel sysroot/toolchain target.

Puoi cambiare toolchain così:

```bash
make am3358-k6 TOOLCHAIN_ROOT=/percorso/toolchain
# oppure
make am3358-k6 CROSS_COMPILE=/percorso/bin/arm-linux-gnueabihf-
```

## Build rapida

Dalla root del progetto:

```bash
make am3358-k49   # target con compatibilità kernel 4.9
make am3358-k6    # target con compatibilità kernel 6.x
```

Output: `Terminal/terminal.bin`

## Doppio funzionamento: kernel 4.9 vs kernel 6.x

Il codice supporta due modalità GPIO:

- **Kernel 4.9**
  - Usa l'interfaccia **sysfs GPIO** (legacy).
  - Non linka `libgpiod`.
  - Build:
    ```bash
    make am3358-k49
    ```

- **Kernel 6.x**
  - Usa **character device GPIO** tramite `libgpiod`.
  - Abilita macro `PMB100_ENABLE_GPIOD=1`.
  - Build:
    ```bash
    make am3358-k6
    ```

Nota: il sorgente mantiene fallback compatibile; la scelta di build forza la modalità consigliata per il kernel target.

## Altri target utili

```bash
make debug         # build debug (kernel 6.x)
make clean         # pulizia oggetti/binario
```

## Documentazione

La documentazione operativa storica del terminale è stata spostata in:

- `docs/readme_terminal.txt`
