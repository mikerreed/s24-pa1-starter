/**
 *  Copyright 2015 Mike Reed
 */

#include "GWindow.h"
#include "../include/GBitmap.h"
#include "../include/GCanvas.h"
#include "../include/GColor.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"

#include <vector>

static const float CORNER_SIZE = 9;

template <typename T> int find_index(const std::vector<T*>& list, T* target) {
    for (int i = 0; i < list.size(); ++i) {
        if (list[i] == target) {
            return i;
        }
    }
    return -1;
}

static GRandom gRand;

static GColor rand_color() {
    return {gRand.nextF(), gRand.nextF(), gRand.nextF(), 0.5f};
}

static GRect make_from_pts(const GPoint& p0, const GPoint& p1) {
    return GRect::LTRB(std::min(p0.x, p1.x), std::min(p0.y, p1.y),
                       std::max(p0.x, p1.x), std::max(p0.y, p1.y));
}

static bool contains(const GRect& rect, float x, float y) {
    return rect.left < x && x < rect.right && rect.top < y && y < rect.bottom;
}

static GRect offset(const GRect& rect, float dx, float dy) {
    return GRect::LTRB(rect.left + dx, rect.top + dy,
                       rect.right + dx, rect.bottom + dy);
}

static bool hit_test(float x0, float y0, float x1, float y1) {
    const float dx = std::abs(x1 - x0);
    const float dy = std::abs(y1 - y0);
    return std::max(dx, dy) <= CORNER_SIZE;
}

static bool in_resize_corner(const GRect& r, float x, float y, GPoint* anchor) {
    if (hit_test(r.left, r.top, x, y)) {
        *anchor = {r.right, r.bottom};
        return true;
    } else if (hit_test(r.right, r.top, x, y)) {
        *anchor = {r.left, r.bottom};
        return true;
    } else if (hit_test(r.right, r.bottom, x, y)) {
        *anchor = {r.left, r.top};
        return true;
    } else if (hit_test(r.left, r.bottom, x, y)) {
        *anchor = {r.right, r.top};
        return true;
    }
    return false;
}

static void draw_corner(GCanvas* canvas, const GColor& c, float x, float y, float dx, float dy) {
    canvas->fillRect(make_from_pts({x, y - 1}, {x + dx, y + 1}), c);
    canvas->fillRect(make_from_pts({x - 1, y}, {x + 1, y + dy}), c);
}

static void draw_hilite(GCanvas* canvas, const GRect& r) {
    const float size = CORNER_SIZE;
    GColor c = {0, 0, 0, 1};
    draw_corner(canvas, c, r.left, r.top, size, size);
    draw_corner(canvas, c, r.left, r.bottom, size, -size);
    draw_corner(canvas, c, r.right, r.top, -size, size);
    draw_corner(canvas, c, r.right, r.bottom, -size, -size);
}

static void constrain_color(GColor* c) {
    c->a = std::max(std::min(c->a, 1.f), 0.1f);
    c->r = std::max(std::min(c->r, 1.f), 0.f);
    c->g = std::max(std::min(c->g, 1.f), 0.f);
    c->b = std::max(std::min(c->b, 1.f), 0.f);
}

class Shape {
public:
    virtual ~Shape() {}
    virtual void draw(GCanvas* canvas) {}
    virtual GRect getRect() = 0;
    virtual void setRect(const GRect&) {}
    virtual GColor getColor() = 0;
    virtual void setColor(const GColor&) {}
};

class RectShape : public Shape {
public:
    RectShape(GColor c) : fColor(c) {
        fRect = GRect::XYWH(0, 0, 0, 0);
    }

    void draw(GCanvas* canvas) override {
        canvas->fillRect(fRect, fColor);
    }

    GRect getRect() override { return fRect; }
    void setRect(const GRect& r) override { fRect = r; }
    GColor getColor() override { return fColor; }
    void setColor(const GColor& c) override { fColor = c; }

private:
    GRect   fRect;
    GColor  fColor;
};

class TestWindow : public GWindow {
    std::vector<Shape*> fList;
    Shape* fShape;
    GColor fBGColor;

public:
    TestWindow(int w, int h) : GWindow(w, h) {
        fBGColor = {1, 1, 1, 1};
        fShape = NULL;
    }

    virtual ~TestWindow() {}
    
protected:
    void onDraw(GCanvas* canvas) override {
        canvas->fillRect(GRect::XYWH(0, 0, 10000, 10000), fBGColor);

        for (auto shape : fList) {
            shape->draw(canvas);
        }
        if (fShape) {
            draw_hilite(canvas, fShape->getRect());
        }
    }

    bool onKeyPress(uint32_t sym) override {
        if (fShape) {
            switch (sym) {
                case SDLK_UP: {
                    int index = find_index(fList, fShape);
                    if (index < fList.size() - 1) {
                        std::swap(fList[index], fList[index + 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DOWN: {
                    int index = find_index(fList, fShape);
                    if (index > 0) {
                        std::swap(fList[index], fList[index - 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DELETE:
                case SDLK_BACKSPACE:
                    this->removeShape(fShape);
                    fShape = NULL;
                    this->updateTitle();
                    this->requestDraw();
                    return true;
                default:
                    break;
            }
        }

        GColor c = fShape ? fShape->getColor() : fBGColor;
        const float delta = 0.1f;
        switch (sym) {
            case 'a': c.a -= delta; break;
            case 'A': c.a += delta; break;
            case 'r': c.r -= delta; break;
            case 'R': c.r += delta; break;
            case 'g': c.g -= delta; break;
            case 'G': c.g += delta; break;
            case 'b': c.b -= delta; break;
            case 'B': c.b += delta; break;
            default:
                return false;
        }
        constrain_color(&c);
        if (fShape) {
            fShape->setColor(c);
        } else {
            c.a = 1;   // need the bg to stay opaque
            fBGColor = c;
        }
        this->updateTitle();
        this->requestDraw();
        return true;
    }

    GClick* onFindClickHandler(GPoint loc) override {
        if (fShape) {
            GPoint anchor;
            if (in_resize_corner(fShape->getRect(), loc.x, loc.y, &anchor)) {
                return new GClick(loc, [this, anchor](GClick* click) {
                    fShape->setRect(make_from_pts(click->curr(), anchor));
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }

        for (int i = fList.size() - 1; i >= 0; --i) {
            if (contains(fList[i]->getRect(), loc.x, loc.y)) {
                fShape = fList[i];
                this->updateTitle();
                return new GClick(loc, [this](GClick* click) {
                    const GPoint curr = click->curr();
                    const GPoint prev = click->prev();
                    fShape->setRect(offset(fShape->getRect(), curr.x - prev.x, curr.y - prev.y));
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }
        
        // else create a new shape
        fShape = new RectShape(rand_color());
        fList.push_back(fShape);
        this->updateTitle();
        return new GClick(loc, [this](GClick* click) {
            if (fShape && GClick::kUp_State == click->state()) {
                if (fShape->getRect().isEmpty()) {
                    this->removeShape(fShape);
                    fShape = NULL;
                    return;
                }
            }
            fShape->setRect(make_from_pts(click->orig(), click->curr()));
            this->updateTitle();
            this->requestDraw();
        });
    }

private:
    void removeShape(Shape* target) {
        assert(target);

        std::vector<Shape*>::iterator it = std::find(fList.begin(), fList.end(), target);
        if (it != fList.end()) {
            fList.erase(it);
        } else {
            assert(!"shape not found?");
        }
    }

    void updateTitle() {
        char buffer[100];
        buffer[0] = ' ';
        buffer[1] = 0;

        GColor c = fBGColor;
        if (fShape) {
            c = fShape->getColor();
        }

        snprintf(buffer, sizeof(buffer), "R:%02X  G:%02X  B:%02X  A:%02X",
                int(c.r * 255), int(c.g * 255), int(c.b * 255), int(c.a * 255));
        this->setTitle(buffer);
    }

    typedef GWindow INHERITED;
};

int main(int argc, char const* const* argv) {
    GWindow* wind = new TestWindow(640, 480);

    return wind->run();
}
