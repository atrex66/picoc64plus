

GPIO_STATE_ADDRESS     = 0xD02F  # 4 bytes: GPIO output state
GPIO_DIRECTION_ADDRESS = 0xD033  # 4 bytes: GPIO direction (1=output)
GPIO_PULLUP_ADDRESS    = 0xD037  # 4 bytes: GPIO pull-up enable

GPIO_NUMBER = 25    # maximum 32 gpio available, numbered 0-31
GPIO_DIRECTION = 1  # 1 = output 0 = input
GPIO_PULLUP = 0     # 1 = pull-up enabled, 0 = pull-up disabled
GPIO_STATE = 1      # 1 = high, 0 = low

gpio_offset =  int(GPIO_NUMBER / 4)
gpio_bit = GPIO_NUMBER % 4

with open("gpio_config.bas", "w") as f:
    f.write(f"10 POKE {GPIO_DIRECTION_ADDRESS}, {GPIO_DIRECTION}\n")
    f.write(f"20 POKE {GPIO_STATE_ADDRESS + gpio_offset}, {gpio_bit}\n")  # Read input state
