def getbin_rec(i):
    if i == 0: return 0, 0, 0
    parent = (i-1)//4
    x, y, rank = getbin_rec(parent)
    rank += 1
    l = 1/(2**rank)

    offset = 4**(rank)//3 # sum of geometric series 4^k
    quad = (i-offset) & 3

    x += (quad%2)*l
    y += (quad//2)*l
    return x, y, rank

def _floorlog4(n):
    return (n.bit_length()+1)//2 - 1

def getbin_iter(i):
    # ty mlugg
    bottom_rank = _floorlog4(3*i + 1)
    bottom_width = 4**bottom_rank
    idx = i - (4**bottom_rank - 1)//3

    x = 0
    y = 0
    for rank in range(1, bottom_rank+1):
        width = 4**rank
        val = idx*width//bottom_width
        quad = val%4

        l = 1/(2**rank)
        x += (quad&1)*l
        y += (quad>>1)*l

    return x, y, bottom_rank

def getbin(i):
    return getbin_iter(i)

import tkinter as tk
from dataclasses import dataclass

width, height = 512, 512
root = tk.Tk()
canvas = tk.Canvas(root, width=width, height=height)
canvas.pack()

@dataclass
class Rect:
    i: int
    a: complex
    b: complex

inserting = Rect(-1, 0.1 + 0.1j, 0.3 + 0.7j)

def getbinrect(i):
    x, y, rank = getbin(i)
    l = 1/2**rank
    a = x+y*1j
    d = l+l*1j
    b = a+d
    return Rect(i, a, b)

def draw(r):
    global color
    a = r.a.real*width + r.a.imag*height*1j
    b = r.b.real*width + r.b.imag*height*1j
    if r.i >= 0:
        fill = "white"
        outline = "black"
    else:
        fill = ""
        outline = "red"
    canvas.create_rectangle(a.real, a.imag, b.real, b.imag, fill=fill, outline=outline)

    if r.i >= 0:
        t = a + (b-a)/2
        canvas.create_text(t.real, t.imag, text=str(r.i))

def qtree():
    tree = [getbinrect(0)]
    draw(tree[0])
    i = 1
    while True:
        draw(inserting)
        yield
        canvas.delete(tk.ALL)

        for j in range(4**(i.bit_length()//2+1)):
            tree.append(getbinrect(i))
            draw(tree[i])
            i += 1

qtree_iter = qtree()
def step():
    next(qtree_iter)
step()

tk.Button(root, text="Step", command=step).pack()
tk.mainloop()
