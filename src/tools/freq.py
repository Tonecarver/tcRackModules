# cents to freq to voltage converter 


import math

FREQ_C4 = 261.6256
FREQ_A4 = 440.0

def showCents(cents):
    freq = FREQ_C4 * pow(2, cents / 1200)
    volts = math.log2(freq / FREQ_C4)
    print(f'-- cents: {cents} freq  = {freq}  volts = {volts}')

showCents(0)
showCents(100)
showCents(1200)
showCents(2400)
showCents(-1200)
showCents(-2400)
showCents(-3600)
showCents(-4800)

