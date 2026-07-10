import math
import pygame

pygame.init()


screen_width = 800
screen_height = 600
screen = pygame.display.set_mode((screen_width, screen_height))


while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            exit()

    screen.fill((0, 0, 0))

    for x in range(screen_width):
        y = int((math.sin(x * 0.05) + 1) * (screen_height / 2))
        screen.set_at((x, y), (255, 255, 255))

    pygame.display.flip()