import sys
import pickle
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QSpinBox, QLabel, QGridLayout, QFileDialog,
    QGroupBox, QMessageBox, QSizePolicy, QComboBox,
    QScrollArea, QCheckBox, QDialog, QDialogButtonBox,
    QRubberBand
)
from PyQt6.QtCore import Qt, QRect, QPoint
from PyQt6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QPixmap, QImage

# -------------------- 256-color palette --------------------
PALETTE_256 = [
    (0x00,0x00,0x00), (0xFF,0xFF,0xFF), (0xa1,0x4d,0x43), (0x6a,0xc1,0xc8),
    (0xa2,0x57,0xa5), (0x5c,0xad,0x5f), (0x4f,0x44,0x9c), (0xcb,0xd6,0x89),
    (0xa3,0x68,0x3a), (0x6e,0x54,0x0b), (0xcc,0x7f,0x76), (0x63,0x63,0x63),
    (0x8b,0x8b,0x8b), (0x9b,0xe3,0x9d), (0x8a,0x7f,0xcd), (0xaf,0xaf,0xaf),
    (0x00,0x00,0x00), (0x10,0x10,0x10), (0x20,0x20,0x20), (0x35,0x35,0x35),
    (0x45,0x45,0x45), (0x55,0x55,0x55), (0x65,0x65,0x65), (0x75,0x75,0x75),
    (0x8A,0x8A,0x8A), (0x9A,0x9A,0x9A), (0xAA,0xAA,0xAA), (0xBA,0xBA,0xBA),
    (0xCA,0xCA,0xCA), (0xDF,0xDF,0xDF), (0xEF,0xEF,0xEF), (0xFF,0xFF,0xFF),
    (0x00,0x00,0xFF), (0x41,0x00,0xFF), (0x82,0x00,0xFF), (0xBE,0x00,0xFF),
    (0xFF,0x00,0xFF), (0xFF,0x00,0xBE), (0xFF,0x00,0x82), (0xFF,0x00,0x41),
    (0xFF,0x00,0x00), (0xFF,0x41,0x00), (0xFF,0x82,0x00), (0xFF,0xBE,0x00),
    (0xFF,0xFF,0x00), (0xBE,0xFF,0x00), (0x82,0xFF,0x00), (0x41,0xFF,0x00),
    (0x00,0xFF,0x00), (0x00,0xFF,0x41), (0x00,0xFF,0x82), (0x00,0xFF,0xBE),
    (0x00,0xFF,0xFF), (0x00,0xBE,0xFF), (0x00,0x82,0xFF), (0x00,0x41,0xFF),
    (0x82,0x82,0xFF), (0x9E,0x82,0xFF), (0xBE,0x82,0xFF), (0xDF,0x82,0xFF),
    (0xFF,0x82,0xFF), (0xFF,0x82,0xDF), (0xFF,0x82,0xBE), (0xFF,0x82,0x9E),
    (0xFF,0x82,0x82), (0xFF,0x9E,0x82), (0xFF,0xBE,0x82), (0xFF,0xDF,0x82),
    (0xFF,0xFF,0x82), (0xDF,0xFF,0x82), (0xBE,0xFF,0x82), (0x9E,0xFF,0x82),
    (0x82,0xFF,0x82), (0x82,0xFF,0x9E), (0x82,0xFF,0xBE), (0x82,0xFF,0xDF),
    (0x82,0xFF,0xFF), (0x82,0xDF,0xFF), (0x82,0xBE,0xFF), (0x82,0x9E,0xFF),
    (0xBA,0xBA,0xFF), (0xCA,0xBA,0xFF), (0xDF,0xBA,0xFF), (0xEF,0xBA,0xFF),
    (0xFF,0xBA,0xFF), (0xFF,0xBA,0xEF), (0xFF,0xBA,0xDF), (0xFF,0xBA,0xCA),
    (0xFF,0xBA,0xBA), (0xFF,0xCA,0xBA), (0xFF,0xDF,0xBA), (0xFF,0xEF,0xBA),
    (0xFF,0xFF,0xBA), (0xEF,0xFF,0xBA), (0xDF,0xFF,0xBA), (0xCA,0xFF,0xBA),
    (0xBA,0xFF,0xBA), (0xBA,0xFF,0xCA), (0xBA,0xFF,0xDF), (0xBA,0xFF,0xEF),
    (0xBA,0xFF,0xFF), (0xBA,0xEF,0xFF), (0xBA,0xDF,0xFF), (0xBA,0xCA,0xFF),
    (0x00,0x00,0x71), (0x1C,0x00,0x71), (0x39,0x00,0x71), (0x55,0x00,0x71),
    (0x71,0x00,0x71), (0x71,0x00,0x55), (0x71,0x00,0x39), (0x71,0x00,0x1C),
    (0x71,0x00,0x00), (0x71,0x1C,0x00), (0x71,0x39,0x00), (0x71,0x55,0x00),
    (0x71,0x71,0x00), (0x55,0x71,0x00), (0x39,0x71,0x00), (0x1C,0x71,0x00),
    (0x00,0x71,0x00), (0x00,0x71,0x1C), (0x00,0x71,0x39), (0x00,0x71,0x55),
    (0x00,0x71,0x71), (0x00,0x55,0x71), (0x00,0x39,0x71), (0x00,0x1C,0x71),
    (0x39,0x39,0x71), (0x45,0x39,0x71), (0x55,0x39,0x71), (0x61,0x39,0x71),
    (0x71,0x39,0x71), (0x71,0x39,0x61), (0x71,0x39,0x55), (0x71,0x39,0x45),
    (0x71,0x39,0x39), (0x71,0x45,0x39), (0x71,0x55,0x39), (0x71,0x61,0x39),
    (0x71,0x71,0x39), (0x61,0x71,0x39), (0x55,0x71,0x39), (0x45,0x71,0x39),
    (0x39,0x71,0x39), (0x39,0x71,0x45), (0x39,0x71,0x55), (0x39,0x71,0x61),
    (0x39,0x71,0x71), (0x39,0x61,0x71), (0x39,0x55,0x71), (0x39,0x45,0x71),
    (0x51,0x51,0x71), (0x59,0x51,0x71), (0x61,0x51,0x71), (0x69,0x51,0x71),
    (0x71,0x51,0x71), (0x71,0x51,0x69), (0x71,0x51,0x61), (0x71,0x51,0x59),
    (0x71,0x51,0x51), (0x71,0x59,0x51), (0x71,0x61,0x51), (0x71,0x69,0x51),
    (0x71,0x71,0x51), (0x69,0x71,0x51), (0x61,0x71,0x51), (0x59,0x71,0x51),
    (0x51,0x71,0x51), (0x51,0x71,0x59), (0x51,0x71,0x61), (0x51,0x71,0x69),
    (0x51,0x71,0x71), (0x51,0x69,0x71), (0x51,0x61,0x71), (0x51,0x59,0x71),
    (0x00,0x00,0x41), (0x10,0x00,0x41), (0x20,0x00,0x41), (0x31,0x00,0x41),
    (0x41,0x00,0x41), (0x41,0x00,0x31), (0x41,0x00,0x20), (0x41,0x00,0x10),
    (0x41,0x00,0x00), (0x41,0x10,0x00), (0x41,0x20,0x00), (0x41,0x31,0x00),
    (0x41,0x41,0x00), (0x31,0x41,0x00), (0x20,0x41,0x00), (0x10,0x41,0x00),
    (0x00,0x41,0x00), (0x00,0x41,0x10), (0x00,0x41,0x20), (0x00,0x41,0x31),
    (0x00,0x41,0x41), (0x00,0x31,0x41), (0x00,0x20,0x41), (0x00,0x10,0x41),
    (0x20,0x20,0x41), (0x28,0x20,0x41), (0x31,0x20,0x41), (0x39,0x20,0x41),
    (0x41,0x20,0x41), (0x41,0x20,0x39), (0x41,0x20,0x31), (0x41,0x20,0x28),
    (0x41,0x20,0x20), (0x41,0x28,0x20), (0x41,0x31,0x20), (0x41,0x39,0x20),
    (0x41,0x41,0x20), (0x39,0x41,0x20), (0x31,0x41,0x20), (0x28,0x41,0x20),
    (0x20,0x41,0x20), (0x20,0x41,0x28), (0x20,0x41,0x31), (0x20,0x41,0x39),
    (0x20,0x41,0x41), (0x20,0x39,0x41), (0x20,0x31,0x41), (0x20,0x28,0x41),
    (0x2D,0x2D,0x41), (0x31,0x2D,0x41), (0x35,0x2D,0x41), (0x3D,0x2D,0x41),
    (0x41,0x2D,0x41), (0x41,0x2D,0x3D), (0x41,0x2D,0x35), (0x41,0x2D,0x31),
    (0x41,0x2D,0x2D), (0x41,0x31,0x2D), (0x41,0x35,0x2D), (0x41,0x3D,0x2D),
    (0x41,0x41,0x2D), (0x3D,0x41,0x2D), (0x35,0x41,0x2D), (0x31,0x41,0x2D),
    (0x2D,0x41,0x2D), (0x2D,0x41,0x31), (0x2D,0x41,0x35), (0x2D,0x41,0x3D),
    (0x2D,0x41,0x41), (0x2D,0x3D,0x41), (0x2D,0x35,0x41), (0x2D,0x31,0x41),
    (0x00,0x00,0x00), (0x00,0x00,0x00), (0x00,0x00,0x00), (0x00,0x00,0x00),
    (0x00,0x00,0x00), (0x00,0x00,0x00), (0x00,0x00,0x00), (0x00,0x00,0x00),
]
PALETTE = [QColor(r, g, b) for (r, g, b) in PALETTE_256]

BACKGROUND_COLORS = [
    ("Black", QColor(0, 0, 0)),
    ("White", QColor(255, 255, 255)),
    ("Gray", QColor(128, 128, 128)),
    ("Light gray", QColor(200, 200, 200)),
    ("Dark blue", QColor(0, 0, 50)),
    ("Green", QColor(0, 50, 0)),
]

# -------------------- Crop dialog (javított) --------------------
class CropDialog(QDialog):
    def __init__(self, pixmap, sprite_width, sprite_height, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Crop Image - Keep Aspect Ratio")
        self.pixmap = pixmap
        self.sprite_width = sprite_width
        self.sprite_height = sprite_height
        self.aspect_ratio = sprite_width / sprite_height

        # Image label inside scroll area
        self.image_label = QLabel()
        self.image_label.setPixmap(pixmap)
        self.image_label.setScaledContents(False)
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop)

        scroll = QScrollArea()
        scroll.setWidget(self.image_label)
        scroll.setWidgetResizable(True)
        scroll.setMinimumSize(400, 300)

        # RubberBand on the label
        self.rubberband = QRubberBand(QRubberBand.Shape.Rectangle, self.image_label)
        self.rubberband.setGeometry(QRect(0, 0, 100, 100))
        self.rubberband.show()

        self.dragging = False
        self.drag_start = None

        layout = QVBoxLayout()
        layout.addWidget(scroll)

        info_label = QLabel("Drag the selection rectangle to choose the crop area.")
        info_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(info_label)

        button_box = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)
        layout.addWidget(button_box)

        self.setLayout(layout)
        self.resize(800, 600)

        # Install event handlers on image_label
        self.image_label.mousePressEvent = self.label_mouse_press
        self.image_label.mouseMoveEvent = self.label_mouse_move
        self.image_label.mouseReleaseEvent = self.label_mouse_release

        # Set initial selection to center of image
        self.update_rubberband_to_center()

    def update_rubberband_to_center(self):
        img_size = self.pixmap.size()
        img_width = img_size.width()
        img_height = img_size.height()

        # Calculate selection size to fit inside image with correct aspect ratio
        if img_width / img_height > self.aspect_ratio:
            sel_height = img_height
            sel_width = int(sel_height * self.aspect_ratio)
            if sel_width > img_width:
                sel_width = img_width
                sel_height = int(sel_width / self.aspect_ratio)
        else:
            sel_width = img_width
            sel_height = int(sel_width / self.aspect_ratio)
            if sel_height > img_height:
                sel_height = img_height
                sel_width = int(sel_height * self.aspect_ratio)

        x = (img_width - sel_width) // 2
        y = (img_height - sel_height) // 2
        self.rubberband.setGeometry(QRect(x, y, sel_width, sel_height))

    def label_mouse_press(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.dragging = True
            self.drag_start = event.position().toPoint()

    def label_mouse_move(self, event):
        if self.dragging and self.drag_start is not None:
            pos = event.position().toPoint()
            rect = self.rubberband.geometry()
            center = rect.center()
            dx = pos.x() - self.drag_start.x()
            dy = pos.y() - self.drag_start.y()
            new_center = center + QPoint(dx, dy)
            rect.moveCenter(new_center)
            # Clamp to image bounds
            img_rect = self.image_label.rect()
            if rect.left() < img_rect.left():
                rect.moveLeft(img_rect.left())
            if rect.top() < img_rect.top():
                rect.moveTop(img_rect.top())
            if rect.right() > img_rect.right():
                rect.moveRight(img_rect.right())
            if rect.bottom() > img_rect.bottom():
                rect.moveBottom(img_rect.bottom())
            self.rubberband.setGeometry(rect)
            self.drag_start = pos

    def label_mouse_release(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.dragging = False
            self.drag_start = None

    def get_cropped_pixmap(self):
        rect = self.rubberband.geometry()
        img_rect = self.image_label.rect()
        rect = rect.intersected(img_rect)
        if rect.width() <= 0 or rect.height() <= 0:
            return None
        return self.pixmap.copy(rect)

# -------------------- Preview widget --------------------
class PreviewWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.parent_window = parent
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.setMinimumSize(100, 100)

    def paintEvent(self, event):
        if self.parent_window is None:
            return
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        w = self.parent_window.canvas.width_px
        h = self.parent_window.canvas.height_px
        rows = self.parent_window.canvas.rows
        grid = self.parent_window.canvas.grid
        trans = self.parent_window.trans_spin.value()

        bg_idx = self.parent_window.bg_combo.currentIndex()
        bg_color = BACKGROUND_COLORS[bg_idx][1]

        widget_w = self.width()
        widget_h = self.height()
        cell_w = widget_w / w
        cell_h = widget_h / rows

        for r in range(rows):
            for c in range(w):
                x = c * cell_w
                y = r * cell_h
                top_idx = grid[r][c][0]
                bot_idx = grid[r][c][1]

                painter.fillRect(QRect(int(x), int(y), int(cell_w), int(cell_h)), bg_color)

                if top_idx != trans:
                    painter.fillRect(
                        QRect(int(x), int(y), int(cell_w), int(cell_h // 2)),
                        PALETTE[top_idx]
                    )
                if bot_idx != trans:
                    painter.fillRect(
                        QRect(int(x), int(y + cell_h // 2), int(cell_w), int(cell_h - cell_h // 2)),
                        PALETTE[bot_idx]
                    )

                #painter.setPen(QPen(QColor(80, 80, 80), 1))
                #painter.drawRect(QRect(int(x), int(y), int(cell_w), int(cell_h)))

        #grid lines for odd height sprites
        #if h % 2 == 1:
        #    r = rows - 1
        #    for c in range(w):
        #        x = c * cell_w
        #        y = r * cell_h
        #        painter.setPen(QPen(QColor(200, 50, 50), 2))
        #        painter.drawLine(int(x), int(y + cell_h // 2), int(x + cell_w), int(y + cell_h))
        #        painter.drawLine(int(x + cell_w), int(y + cell_h // 2), int(x), int(y + cell_h))

    def update_preview(self):
        self.update()

# -------------------- Canvas widget (editor) --------------------
class CanvasWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(300, 300)
        self.setMouseTracking(True)

        self.width_px = 40
        self.height_px = 48
        self.rows = (self.height_px + 1) // 2

        self.grid = [[[0, 0] for _ in range(self.width_px)] for _ in range(self.rows)]
        self.parent_window = parent

    def set_size(self, w, h):
        self.width_px = w
        self.height_px = h
        self.rows = (h + 1) // 2
        self.grid = [[[0, 0] for _ in range(w)] for _ in range(self.rows)]
        self.update()

    def get_cell_and_half(self, pos):
        cell_w = self.width() / self.width_px
        cell_h = self.height() / self.rows
        col = int(pos.x() // cell_w)
        row = int(pos.y() // cell_h)
        if 0 <= row < self.rows and 0 <= col < self.width_px:
            rel_y = (pos.y() - row * cell_h) / cell_h
            is_top = rel_y < 0.5
            return row, col, is_top
        return None, None, None

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        cell_w = self.width() / self.width_px
        cell_h = self.height() / self.rows
        trans = self.parent_window.trans_spin.value()

        for r in range(self.rows):
            for c in range(self.width_px):
                x = c * cell_w
                y = r * cell_h
                top_idx, bot_idx = self.grid[r][c]

                painter.fillRect(QRect(int(x), int(y), int(cell_w), int(cell_h)), QColor(60, 60, 60))

                if top_idx != trans:
                    painter.fillRect(
                        QRect(int(x), int(y), int(cell_w), int(cell_h // 2)),
                        PALETTE[top_idx]
                    )
                else:
                    brush = QBrush(QColor(80, 80, 80))
                    painter.fillRect(
                        QRect(int(x), int(y), int(cell_w), int(cell_h // 2)),
                        brush
                    )
                    painter.setPen(QPen(QColor(40, 40, 40), 1))
                    painter.drawLine(int(x), int(y), int(x + cell_w), int(y + cell_h // 2))
                    painter.drawLine(int(x + cell_w), int(y), int(x), int(y + cell_h // 2))

                if bot_idx != trans:
                    painter.fillRect(
                        QRect(int(x), int(y + cell_h // 2), int(cell_w), int(cell_h - cell_h // 2)),
                        PALETTE[bot_idx]
                    )
                else:
                    brush = QBrush(QColor(80, 80, 80))
                    painter.fillRect(
                        QRect(int(x), int(y + cell_h // 2), int(cell_w), int(cell_h - cell_h // 2)),
                        brush
                    )
                    painter.setPen(QPen(QColor(40, 40, 40), 1))
                    mid_y = int(y + cell_h // 2)
                    painter.drawLine(int(x), mid_y, int(x + cell_w), int(y + cell_h))
                    painter.drawLine(int(x + cell_w), mid_y, int(x), int(y + cell_h))

                painter.setPen(QPen(QColor(120, 120, 120), 1))
                painter.drawRect(QRect(int(x), int(y), int(cell_w), int(cell_h)))

        if self.height_px % 2 == 1:
            r = self.rows - 1
            for c in range(self.width_px):
                x = c * cell_w
                y = r * cell_h
                painter.setPen(QPen(QColor(200, 50, 50), 2))
                painter.drawLine(int(x), int(y + cell_h // 2), int(x + cell_w), int(y + cell_h))
                painter.drawLine(int(x + cell_w), int(y + cell_h // 2), int(x), int(y + cell_h))

    def mousePressEvent(self, event):
        row, col, is_top = self.get_cell_and_half(event.position())
        if row is None or col is None:
            return

        trans = self.parent_window.trans_spin.value()
        color = self.parent_window.current_color

        if event.button() == Qt.MouseButton.LeftButton:
            if is_top:
                self.grid[row][col][0] = color
            else:
                self.grid[row][col][1] = color
            self.update()
            self.parent_window.update_preview()

        elif event.button() == Qt.MouseButton.RightButton:
            if is_top:
                self.grid[row][col][0] = trans
            else:
                self.grid[row][col][1] = trans
            self.update()
            self.parent_window.update_preview()

    def mouseMoveEvent(self, event):
        if not (event.buttons() & (Qt.MouseButton.LeftButton | Qt.MouseButton.RightButton)):
            return
        row, col, is_top = self.get_cell_and_half(event.position())
        if row is None or col is None:
            return

        trans = self.parent_window.trans_spin.value()
        color = self.parent_window.current_color

        if event.buttons() & Qt.MouseButton.LeftButton:
            if is_top:
                self.grid[row][col][0] = color
            else:
                self.grid[row][col][1] = color
        elif event.buttons() & Qt.MouseButton.RightButton:
            if is_top:
                self.grid[row][col][0] = trans
            else:
                self.grid[row][col][1] = trans
        self.update()
        self.parent_window.update_preview()

# -------------------- Main window --------------------
class SpriteEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Python Sprite Editor - Terminal (256 colors)")
        self.setGeometry(100, 100, 1200, 800)

        self.current_color = 1

        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QHBoxLayout(central)

        # ---------- Left side: canvas ----------
        left_panel = QVBoxLayout()
        self.canvas = CanvasWidget(self)
        self.canvas.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        left_panel.addWidget(self.canvas)

        size_layout = QHBoxLayout()
        size_layout.addWidget(QLabel("Width:"))
        self.width_spin = QSpinBox()
        self.width_spin.setRange(1, 256)
        self.width_spin.setValue(40)
        self.width_spin.valueChanged.connect(self.on_size_changed)
        size_layout.addWidget(self.width_spin)

        size_layout.addWidget(QLabel("Height:"))
        self.height_spin = QSpinBox()
        self.height_spin.setRange(1, 256)
        self.height_spin.setValue(48)
        self.height_spin.valueChanged.connect(self.on_size_changed)
        size_layout.addWidget(self.height_spin)
        size_layout.addStretch()
        left_panel.addLayout(size_layout)

        trans_layout = QHBoxLayout()
        trans_layout.addWidget(QLabel("Transparent color index:"))
        self.trans_spin = QSpinBox()
        self.trans_spin.setRange(0, 255)
        self.trans_spin.setValue(0)
        self.trans_spin.valueChanged.connect(self.on_transparency_changed)
        trans_layout.addWidget(self.trans_spin)
        trans_layout.addStretch()
        left_panel.addLayout(trans_layout)

        main_layout.addLayout(left_panel, stretch=2)

        # ---------- Right side ----------
        right_panel = QVBoxLayout()

        # Palette
        palette_group = QGroupBox("Palette (left=draw, right=erase)")
        palette_layout = QVBoxLayout()
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setMaximumHeight(300)
        palette_widget = QWidget()
        palette_grid = QGridLayout(palette_widget)
        palette_grid.setSpacing(2)

        self.color_buttons = []
        for idx, color in enumerate(PALETTE):
            btn = QPushButton()
            btn.setFixedSize(20, 20)
            btn.setStyleSheet(
                f"background-color: rgb({color.red()}, {color.green()}, {color.blue()});"
                f"border: 1px solid gray;"
            )
            btn.setToolTip(f"Index {idx}")
            btn.clicked.connect(lambda checked, i=idx: self.select_color(i))
            palette_grid.addWidget(btn, idx // 16, idx % 16)
            self.color_buttons.append(btn)

        scroll.setWidget(palette_widget)
        palette_layout.addWidget(scroll)

        self.selected_label = QLabel("Selected color: 1 - white")
        self.selected_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        palette_layout.addWidget(self.selected_label)
        palette_group.setLayout(palette_layout)
        right_panel.addWidget(palette_group)

        # ---- Fájlkezelő gombok ----
        file_btn_layout = QHBoxLayout()
        save_btn = QPushButton("Save Sprite...")
        save_btn.clicked.connect(self.save_sprite)
        load_btn = QPushButton("Load Sprite...")
        load_btn.clicked.connect(self.load_sprite)
        file_btn_layout.addWidget(save_btn)
        file_btn_layout.addWidget(load_btn)
        right_panel.addLayout(file_btn_layout)

        # ---- Képbetöltés ----
        img_load_layout = QHBoxLayout()
        self.aspect_check = QCheckBox("Keep aspect ratio")
        self.aspect_check.setChecked(True)
        img_load_layout.addWidget(self.aspect_check)
        img_load_layout.addStretch()
        right_panel.addLayout(img_load_layout)

        load_img_btn = QPushButton("Load Image...")
        load_img_btn.clicked.connect(self.load_image)
        right_panel.addWidget(load_img_btn)

        # Graphical preview
        preview_group = QGroupBox("Preview (graphical, background selectable)")
        preview_layout = QVBoxLayout()

        bg_layout = QHBoxLayout()
        bg_layout.addWidget(QLabel("Background color:"))
        self.bg_combo = QComboBox()
        for name, _ in BACKGROUND_COLORS:
            self.bg_combo.addItem(name)
        self.bg_combo.currentIndexChanged.connect(self.update_preview)
        bg_layout.addWidget(self.bg_combo)
        bg_layout.addStretch()
        preview_layout.addLayout(bg_layout)

        self.preview_widget = PreviewWidget(self)
        preview_layout.addWidget(self.preview_widget)
        preview_group.setLayout(preview_layout)
        right_panel.addWidget(preview_group, stretch=2)

        # Export button
        export_btn = QPushButton("Export C code (.h)")
        export_btn.clicked.connect(self.export_c_code)
        right_panel.addWidget(export_btn)

        right_panel.addStretch()
        main_layout.addLayout(right_panel, stretch=1)

        self.on_size_changed()
        self.select_color(1)

    # ---------- Color selection ----------
    def select_color(self, idx):
        self.current_color = idx
        color = PALETTE[idx]
        self.selected_label.setText(f"Selected color: {idx} - RGB({color.red()},{color.green()},{color.blue()})")
        for i, btn in enumerate(self.color_buttons):
            if i == idx:
                btn.setStyleSheet(btn.styleSheet() + "border: 2px solid white;")
            else:
                btn.setStyleSheet(btn.styleSheet().replace("border: 2px solid white;", "border: 1px solid gray;"))

    # ---------- Size change ----------
    def on_size_changed(self):
        w = self.width_spin.value()
        h = self.height_spin.value()
        self.canvas.set_size(w, h)
        self.update_preview()

    def on_transparency_changed(self):
        self.canvas.update()
        self.update_preview()

    def update_preview(self):
        self.preview_widget.update_preview()

    # ---------- Save / Load sprite ----------
    def save_sprite(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Sprite", "sprite.spr", "Sprite files (*.spr)"
        )
        if not file_path:
            return

        data = {
            'width': self.canvas.width_px,
            'height': self.canvas.height_px,
            'transparency': self.trans_spin.value(),
            'grid': self.canvas.grid
        }
        try:
            with open(file_path, 'wb') as f:
                pickle.dump(data, f)
            QMessageBox.information(self, "Success", f"Sprite saved to:\n{file_path}")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to save sprite:\n{e}")

    def load_sprite(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Load Sprite", "", "Sprite files (*.spr)"
        )
        if not file_path:
            return

        try:
            with open(file_path, 'rb') as f:
                data = pickle.load(f)

            required = ('width', 'height', 'transparency', 'grid')
            if not all(k in data for k in required):
                raise ValueError("Invalid sprite file: missing required data.")

            w = data['width']
            h = data['height']
            trans = data['transparency']
            grid = data['grid']

            rows = (h + 1) // 2
            if len(grid) != rows or any(len(row) != w for row in grid):
                raise ValueError("Grid dimensions do not match width/height.")

            self.width_spin.setValue(w)
            self.height_spin.setValue(h)
            self.trans_spin.setValue(trans)
            self.canvas.set_size(w, h)
            self.canvas.grid = grid
            self.canvas.update()
            self.update_preview()
            QMessageBox.information(self, "Success", f"Sprite loaded from:\n{file_path}")

        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to load sprite:\n{e}")

    # ---------- Load image ----------
    def load_image(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Load Image",
            "",
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.webp)"
        )
        if not file_path:
            return

        pixmap = QPixmap(file_path)
        if pixmap.isNull():
            QMessageBox.critical(self, "Error", "Failed to load image.")
            return

        w = self.width_spin.value()
        h = self.height_spin.value()

        # Ha be van pipálva a képarány megtartása, crop dialog
        if self.aspect_check.isChecked():
            crop_dialog = CropDialog(pixmap, w, h, self)
            if crop_dialog.exec() != QDialog.DialogCode.Accepted:
                return
            cropped = crop_dialog.get_cropped_pixmap()
            if cropped is None or cropped.isNull():
                QMessageBox.critical(self, "Error", "Crop selection is empty.")
                return
            # Skálázzuk a kivágott részt a sprite méretére
            pixmap = cropped.scaled(
                w, h,
                Qt.AspectRatioMode.IgnoreAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            )
        else:
            # Régi módszer: aránytalan skálázás
            pixmap = pixmap.scaled(
                w, h,
                Qt.AspectRatioMode.IgnoreAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            )

        # Átalakítás QImage-re és feltöltés a gridbe
        image = pixmap.toImage()
        trans = self.trans_spin.value()
        rows = self.canvas.rows
        grid = self.canvas.grid

        # Töröljük a gridet
        for r in range(rows):
            for c in range(w):
                grid[r][c][0] = trans
                grid[r][c][1] = trans

        # Feltöltés
        for y in range(h):
            for x in range(w):
                color = image.pixelColor(x, y)
                alpha = color.alpha()
                if alpha < 128:
                    idx = trans
                else:
                    idx = self.find_closest_palette_index(color.red(), color.green(), color.blue())
                row = y // 2
                is_top = (y % 2) == 0
                if row < rows:
                    if is_top:
                        grid[row][x][0] = idx
                    else:
                        grid[row][x][1] = idx

        self.canvas.update()
        self.update_preview()

    def find_closest_palette_index(self, r, g, b):
        best_idx = 0
        best_dist = 3 * 255 * 255 + 1
        for i, col in enumerate(PALETTE):
            dr = r - col.red()
            dg = g - col.green()
            db = b - col.blue()
            dist = dr*dr + dg*dg + db*db
            if dist < best_dist:
                best_dist = dist
                best_idx = i
                if dist == 0:
                    break
        return best_idx

    # ---------- Export C code ----------
    def export_c_code(self):
        w = self.canvas.width_px
        h = self.canvas.height_px
        rows = self.canvas.rows
        grid = self.canvas.grid
        trans = self.trans_spin.value()

        arr = [0] * (h * w)

        idx = 0
        for r in range(rows):
            for c in range(w):
                arr[idx] = grid[r][c][0]
                idx += 1
            for c in range(w):
                arr[idx] = grid[r][c][1]
                idx += 1

        print("pixel array:", arr)

        path, _ = QFileDialog.getSaveFileName(
            self, "Save C header", "sprite_data.h", "C Headers (*.h)"
        )
        if not path:
            return

        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(f"// Automatically generated sprite data\n")
                f.write(f"// Width: {w}, Height: {h} (in half-pixels)\n")
                f.write(f"// Character rows: {h}\n")
                f.write(f"// Transparent color: {trans}\n\n")

                f.write(f"#ifndef SPRITE_DATA_H\n#define SPRITE_DATA_H\n\n")
                f.write(f'#include <stdint.h>\n\n')

                f.write(f'#define SPRITE_WIDTH  {w}\n')
                f.write(f'#define SPRITE_HEIGHT {h}\n')
                f.write(f'#define SPRITE_TRANSPARENCY {trans}\n\n')

                f.write(f'static const uint8_t sprite_data[{h*w}] = {{\n')
                for i in range(0, h*w, w):
                    chunk = arr[i:i+w]
                    f.write(f'    ' + ','.join(f'0x{x:02X}' for x in chunk) + (',' if i+w < h*w else '') + '\n')
                f.write(f'}};\n\n')
                f.write(f'SpriteAsset_t my_sprite = {{\n')
                f.write(f'    .width = SPRITE_WIDTH,\n')
                f.write(f'    .height = SPRITE_HEIGHT,\n')
                f.write(f'    .x = 0,\n')
                f.write(f'    .y = 0,\n')
                f.write(f'    .enabled = 1,\n')
                f.write(f'    .transparency_color = SPRITE_TRANSPARENCY,\n')
                f.write(f'    .pixel_color = (uint8_t*)sprite_data,\n')
                f.write(f'}};\n\n')
                f.write(f'#endif // SPRITE_DATA_H\n')

            tp = 0x8000
            line_increment = 5

            with open(path + ".bas", "w", encoding="utf-8") as f:
                start_line = 10
                f.write(f'{start_line} REM Sprite data loader\n')
                start_line += line_increment
                f.write(f'{start_line} REM Width: {w}, Height: {h}\n')
                start_line += line_increment
                f.write(f'{start_line} REM Character rows: {rows}\n')
                start_line += line_increment
                f.write(f'{start_line} REM Transparent color: {trans}\n')
                start_line += line_increment
                f.write(f'{start_line} REM Total data: {h * w}\n')
                start_line += line_increment
                f.write(f'{start_line} GOSUB 1005\n')
                start_line += line_increment
                f.write(f'{start_line} POKE $A002,1\n') # enable sprite
                start_line = 1000
                f.write(f'{start_line} END\n')
                start_line += line_increment
                f.write(f'{start_line} POKE $A000, ${w:02X}\n')
                start_line += line_increment
                f.write(f'{start_line} POKE $A001, ${h:02X}\n')
                start_line += line_increment
                f.write(f"{start_line} POKE $A002,0\n") # disable sprite
                start_line += line_increment
                f.write(f'{start_line} POKE $A008, ${tp & 0xFF:02X}\n')
                start_line += line_increment
                f.write(f'{start_line} POKE $A009, ${tp >> 8 & 0xFF:02X}\n')
                start_line += line_increment
                f.write(f'{start_line} FOR I = 1 TO {h*w}\n')
                start_line += line_increment
                f.write(f'{start_line} READ A\n')
                start_line += line_increment
                f.write(f'{start_line} POKE ${tp:04X} + I - 1, A\n')
                start_line += line_increment
                f.write(f'{start_line} NEXT I\n')
                start_line += line_increment
                f.write(f'{start_line} RETURN\n')
                start_line += line_increment
                f.write(f'{start_line} REM Top pixel color data\n')
                start_line += line_increment
                data_counter = 0
                for i in range(0, h*w, 10):
                    chunk = arr[i:i+10]
                    f.write(f'{start_line} DATA ' + ','.join(f'{x}' for x in chunk) + '\n')
                    data_counter += len(chunk)
                    start_line += line_increment
                f.write(f'{start_line} REM Total data bytes: {data_counter}\n')
                

            total_bytes = 2 * rows * w
            QMessageBox.information(
                self,
                "Success",
                f"C code exported to:\n{path}\n\n"
                f"Sprite size: {w}×{h} half-pixels\n"
                f"Character rows: {rows}\n"
                f"Total data: {h * w} bytes\n"
                f"Per array: {h * w} bytes"
            )



        except Exception as e:
            QMessageBox.critical(self, "Error", f"Export error:\n{e}")

# -------------------- Start --------------------
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SpriteEditor()
    window.show()
    sys.exit(app.exec())