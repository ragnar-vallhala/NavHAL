#!/usr/bin/env python3
"""A module that owns an interrupt line owns its vector.

Nothing outside a driver may put its handler in another driver's IRQ slot.
There is one callback slot per IRQ, so a second claimant does not fail -- it
silently unhooks the first, and the loser never hears that its transfer
finished. That shipped: i2c and sdio both reached past the DMA module and into
DMA stream vectors, and a caller doing the same would have displaced them.

The rule this checks is the narrow, greppable half of the invariant:

    a call to hal_interrupt_attach_callback naming an IRQ literal may only
    appear in the module that owns that peripheral.

A driver asking the owning module instead -- hal_dma_attach_callback and
friends -- is what this pushes callers towards. Calls whose IRQ is a variable
are reported, not failed: the owning module routinely computes its own vector
(uart picks USART2 or USART6 at run time), and this checker does not pretend
to evaluate C.

Exit 0 when clean, 1 on a violation.
"""
import os, re, sys

ROOTS = ("src/vendor", "src/arch")
CALL = re.compile(r"hal_interrupt_attach_callback\s*\(\s*([^,]+?)\s*,")

# IRQ-name prefix -> the module directory that owns it.
OWNER = [
    (r"^DMA\d_Stream\d+_IRQn$",            "dma"),
    (r"^(USART|UART)\d+_IRQn$",            "uart"),
    (r"^HAL_IRQ_USART_",                   "uart"),
    (r"^OTG_(FS|HS)_IRQn$",                "usb"),
    (r"^RTC_(Alarm|WKUP)_IRQn$",           "rtc"),
    (r"^SDIO_IRQn$",                       "sdio"),
    (r"^TIM\d+.*_IRQn$",                   "timer"),
    (r"^HAL_IRQ_TIMER\d*$",                "timer"),
    (r"^ADC\d*_IRQn$",                     "adc"),
    (r"^I2C\d+_(EV|ER)_IRQn$",             "i2c"),
    (r"^SPI\d+_IRQn$",                     "spi"),
    (r"^ETH_IRQn$",                        "eth"),
    (r"^EXTI\d*_?\w*_IRQn$",               "gpio"),
    (r"^HAL_IRQ_KEYBOARD$",                "ps2"),
]

def owner_of(irq):
    for pat, mod in OWNER:
        if re.match(pat, irq):
            return mod
    return None

def module_of(path):
    parts = path.split(os.sep)
    return parts[-2] if len(parts) >= 2 else ""

def main():
    violations, unresolved, checked = [], [], 0
    for root in ROOTS:
        for dirpath, _, files in os.walk(root):
            for fn in files:
                if not fn.endswith(".c"):
                    continue
                path = os.path.join(dirpath, fn)
                # the interrupt backend is where the table lives
                if os.sep + "interrupt" + os.sep in path:
                    continue
                with open(path, errors="replace") as fh:
                    for n, line in enumerate(fh, 1):
                        m = CALL.search(line)
                        if not m:
                            continue
                        arg = m.group(1).replace("(hal_irq_t)", "").strip()
                        checked += 1
                        own = owner_of(arg)
                        mod = module_of(path)
                        if own is None:
                            unresolved.append((path, n, arg, mod))
                        elif own != mod:
                            violations.append((path, n, arg, own, mod))

    print(f"attach sites outside the interrupt backend : {checked}")
    print(f"  resolved to an owning module             : {checked - len(unresolved)}")
    print(f"  IRQ not a literal (reported, not failed) : {len(unresolved)}")
    for path, n, arg, mod in unresolved:
        print(f"      {path}:{n}  {arg}  (in {mod})")
    if violations:
        print(f"\n{len(violations)} module(s) reaching into another's vector:")
        for path, n, arg, own, mod in violations:
            print(f"  {path}:{n}")
            print(f"      attaches to {arg}, owned by {own}, from {mod}")
            print(f"      ask {own} instead -- hal_{own}_attach_callback, or the"
                  f" call that takes a callback")
        return 1
    print("\nno module reaches into another's vector.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
