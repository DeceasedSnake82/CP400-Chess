// ============================================================================
//  Chess Ai - Player vs Bot - Designed for CP400 Calcuator
//
//  Board: Light/dark brown squares, pure white / pure black pieces (outlined)
//  Menu:  Toggle White/Black side, depth +/-, Start
//  Game:  Tap piece -> highlight + legal-move dots, tap dest -> move
//         full legality: pins, checks, castling, en passant, promotion(Only to
//         queen but who has ever actually promoted to anything else in a game
//         except maybe a knight)
//  Bot:   Negamax + alpha-beta, iterative deepening, TT, MVV-LVA ordering,
//         quiescence search, simple PST evaluation. Depth selectable (1 to 6).
// ============================================================================

//For running on calc

#include <appdef.h>
#include <sdk/calc/calc.h>
#include <sdk/os/lcd.h>
#include <sdk/os/input.h>
#include <sdk/os/mem.h>
#include <sdk/os/string.h>
#include <stdint.h>
#include <stdbool.h>

//For running on emulator

//#include "calculator_sdk.h"
//#include <string.h>
//#include <stdlib.h>

APP_NAME("Chess")
APP_DESCRIPTION("A simple chess ai to play against")
APP_AUTHOR("DeceasedSnake82")
APP_VERSION("1.1.1")

// ----------------------------------------------------------------------------
// Sprites (16x16, 1bpp packed into 16 bits/row, bit 15 = leftmost pixel)
// ----------------------------------------------------------------------------
static const uint16_t sprite_pawn[16]   = { 0x0000, 0x03C0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x03C0, 0x07E0, 0x07E0, 0x03C0, 0x03C0, 0x03C0, 0x07E0, 0x1FF8, 0x1FF8, 0x0000 };
static const uint16_t sprite_knight[16] = { 0x0000, 0x0080, 0x03C0, 0x07E0, 0x0FE0, 0x1FE0, 0x1CE0, 0x18E0, 0x01E0, 0x03C0, 0x03C0, 0x03C0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000 };
static const uint16_t sprite_bishop[16] = { 0x0000, 0x0180, 0x0180, 0x03C0, 0x07E0, 0x07E0, 0x03C0, 0x07E0, 0x07E0, 0x03C0, 0x03C0, 0x03C0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000 };
static const uint16_t sprite_rook[16]   = { 0x0000, 0x1998, 0x1998, 0x1FF8, 0x1FF8, 0x07E0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000 };
static const uint16_t sprite_queen[16]  = { 0x0000, 0x0990, 0x0DB0, 0x0FF0, 0x07E0, 0x07E0, 0x03C0, 0x07E0, 0x07E0, 0x03C0, 0x03C0, 0x03C0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000 };
static const uint16_t sprite_king[16]   = { 0x0000, 0x0180, 0x03C0, 0x03C0, 0x0180, 0x07E0, 0x03C0, 0x07E0, 0x07E0, 0x03C0, 0x03C0, 0x03C0, 0x0FF0, 0x1FF8, 0x1FF8, 0x0000 };

static const uint16_t* const kPieceSprites[6] = {
    sprite_pawn, sprite_knight, sprite_bishop, sprite_rook, sprite_queen, sprite_king
};

// ----------------------------------------------------------------------------
// Colors
// ----------------------------------------------------------------------------
static inline uint16_t COL(uint8_t r, uint8_t g, uint8_t b) { return color(r, g, b); }

#define COLOR_LIGHT_SQ   COL(0xEE, 0xD6, 0xB4)
#define COLOR_DARK_SQ    COL(0x8B, 0x5A, 0x2B)
#define COLOR_WHITE_PC   COL(0xFF, 0xFF, 0xFF)
#define COLOR_BLACK_PC   COL(0x00, 0x00, 0x00)
#define COLOR_OUTLINE_W  COL(0x20, 0x20, 0x20)
#define COLOR_OUTLINE_B  COL(0xE0, 0xE0, 0xE0)
#define COLOR_SELECT     COL(0x30, 0xC0, 0x30)
#define COLOR_LASTMOVE   COL(0xE0, 0xC0, 0x40)
#define COLOR_DOT        COL(0x30, 0x30, 0x30)
#define COLOR_CAPTUREDOT COL(0xC0, 0x30, 0x30)
#define COLOR_CHECK      COL(0xFF, 0x30, 0x30)
#define COLOR_BG         COL(0x22, 0x22, 0x28)
#define COLOR_TEXT       COL(0xFF, 0xFF, 0xFF)
#define COLOR_BTN        COL(0x44, 0x44, 0x55)
#define COLOR_BTN_HI     COL(0x66, 0x66, 0x88)
#define COLOR_BTN_TEXT   COL(0xFF, 0xFF, 0xFF)
#define COLOR_PROGRESS   COL(0x30, 0x80, 0xFF)

// ----------------------------------------------------------------------------
// Tiny 5x7 font (bitmap font, 1 bit/pixel) (I couldn't figure out how to use
// CP400 ui lib lol)
// ----------------------------------------------------------------------------
struct Glyph { char c; uint8_t rows[7]; };
static const Glyph FONT[] = {
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D', {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}},
    {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M', {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'N', {0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W', {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}},
    {'X', {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y', {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'a', {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}},
    {'c', {0x00,0x00,0x0E,0x10,0x10,0x11,0x0E}},
    {'e', {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}},
    {'h', {0x10,0x10,0x16,0x19,0x11,0x11,0x11}},
    {'i', {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}},
    {'k', {0x10,0x10,0x12,0x14,0x18,0x14,0x12}},
    {'n', {0x00,0x00,0x16,0x19,0x11,0x11,0x11}},
    {'o', {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}},
    {'p', {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}},
    {'s', {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E}},
    {'t', {0x08,0x08,0x1C,0x08,0x08,0x09,0x06}},
    {'u', {0x00,0x00,0x11,0x11,0x11,0x13,0x0D}},
    {'v', {0x00,0x00,0x11,0x11,0x11,0x0A,0x04}},
    {'w', {0x00,0x00,0x11,0x11,0x15,0x15,0x0A}},
    {'x', {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}},
    {'y', {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E}},
    {'d', {0x01,0x01,0x0D,0x13,0x11,0x11,0x0F}},
    {'g', {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E}},
    {'l', {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'m', {0x00,0x00,0x1A,0x15,0x15,0x15,0x15}},
    {'b', {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E}},
    {'f', {0x06,0x09,0x08,0x1C,0x08,0x08,0x08}},
    {'r', {0x00,0x00,0x16,0x19,0x10,0x10,0x10}},
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3', {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},
    {'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5', {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
    {'6', {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
    {'7', {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9', {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
    {'+', {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
    {'.', {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}},
    {':', {0x00,0x04,0x00,0x00,0x00,0x04,0x00}},
    {'!', {0x04,0x04,0x04,0x04,0x04,0x00,0x04}},
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
};
static const int FONT_N = sizeof(FONT) / sizeof(FONT[0]);

static void drawChar(int x, int y, char c, uint16_t col, int scale) {
    for (int i = 0; i < FONT_N; ++i) {
        if (FONT[i].c == c) {
            for (int row = 0; row < 7; ++row) {
                uint8_t bits = FONT[i].rows[row];
                for (int colb = 0; colb < 5; ++colb) {
                    if (bits & (1 << (4 - colb))) {
                        for (int sy = 0; sy < scale; ++sy)
                            for (int sx = 0; sx < scale; ++sx)
                                setPixel(x + colb * scale + sx, y + row * scale + sy, col);
                    }
                }
            }
            return;
        }
    }
}

static void drawText(int x, int y, const char* s, uint16_t col, int scale) {
    int cx = x;
    while (*s) {
        drawChar(cx, y, *s, col, scale);
        cx += 6 * scale;
        ++s;
    }
}

static int textWidth(const char* s, int scale) {
    int n = 0;
    while (s[n]) ++n;
    return n * 6 * scale - scale; // minus trailing gap
}

static void fillRect(int x, int y, int w, int h, uint16_t col) {
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            setPixel(x + i, y + j, col);
}

static void drawRectOutline(int x, int y, int w, int h, uint16_t col, int thick) {
    for (int t = 0; t < thick; ++t) {
        for (int i = 0; i < w; ++i) {
            setPixel(x + i, y + t, col);
            setPixel(x + i, y + h - 1 - t, col);
        }
        for (int j = 0; j < h; ++j) {
            setPixel(x + t, y + j, col);
            setPixel(x + w - 1 - t, y + j, col);
        }
    }
}

static void fillCircle(int cx, int cy, int r, uint16_t col) {
    for (int y = -r; y <= r; ++y)
        for (int x = -r; x <= r; ++x)
            if (x * x + y * y <= r * r)
                setPixel(cx + x, cy + y, col);
}

// ----------------------------------------------------------------------------
// Button widget
// ----------------------------------------------------------------------------
struct Button {
    int x, y, w, h;
    const char* label;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    void draw(bool highlighted) const {
        fillRect(x, y, w, h, highlighted ? COLOR_BTN_HI : COLOR_BTN);
        drawRectOutline(x, y, w, h, COLOR_TEXT, 1);
        int tw = textWidth(label, 2);
        int tx = x + (w - tw) / 2;
        int ty = y + (h - 14) / 2;
        drawText(tx, ty, label, COLOR_BTN_TEXT, 2);
    }
};

// ============================================================================
//  CHESS ENGINE CORE
// ============================================================================

#define EMPTY 0
#define PT_PAWN 1
#define PT_KNIGHT 2
#define PT_BISHOP 3
#define PT_ROOK 4
#define PT_QUEEN 5
#define PT_KING 6
#define COL_WHITE 0
#define COL_BLACK 1

static inline uint8_t makePiece(int c, int t) { return (uint8_t)((c << 3) | t); }
static inline int pieceType(uint8_t p) { return p & 7; }
static inline int pieceColor(uint8_t p) { return (p >> 3) & 1; }
static inline bool isEmpty(uint8_t p) { return p == 0; }

static inline int sqOf(int file, int rank) { return rank * 8 + file; }
static inline int fileOf(int sq) { return sq & 7; }
static inline int rankOf(int sq) { return sq >> 3; }

struct Move {
    uint8_t from, to;
    uint8_t promo;      
    uint8_t flags;       
};
#define MFLAG_CAPTURE   0x01
#define MFLAG_DOUBLE    0x02
#define MFLAG_EP        0x04
#define MFLAG_CASTLE_K  0x08
#define MFLAG_CASTLE_Q  0x10

struct Undo {
    uint8_t captured;
    uint8_t epSquareBefore;
    uint8_t castleRightsBefore;
    Move move;
    uint8_t halfmoveBefore;
};

struct Board {
    uint8_t sq[64];
    int sideToMove;         
    uint8_t castleRights;   
    int epSquare;           
    int halfmoveClock;
    int kingSq[2];
};

#define CR_WK 0x1
#define CR_WQ 0x2
#define CR_BK 0x4
#define CR_BQ 0x8

static Board board;

static void setupStartPosition(Board& b) {
    for (int i = 0; i < 64; ++i) b.sq[i] = EMPTY;
    const int backRank[8] = { PT_ROOK, PT_KNIGHT, PT_BISHOP, PT_QUEEN, PT_KING, PT_BISHOP, PT_KNIGHT, PT_ROOK };
    for (int f = 0; f < 8; ++f) {
        b.sq[sqOf(f, 0)] = makePiece(COL_WHITE, backRank[f]);
        b.sq[sqOf(f, 1)] = makePiece(COL_WHITE, PT_PAWN);
        b.sq[sqOf(f, 6)] = makePiece(COL_BLACK, PT_PAWN);
        b.sq[sqOf(f, 7)] = makePiece(COL_BLACK, backRank[f]);
    }
    b.sideToMove = COL_WHITE;
    b.castleRights = CR_WK | CR_WQ | CR_BK | CR_BQ;
    b.epSquare = -1;
    b.halfmoveClock = 0;
    b.kingSq[COL_WHITE] = sqOf(4, 0);
    b.kingSq[COL_BLACK] = sqOf(4, 7);
}

static const int knightDeltas[8][2] = { {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2} };
static const int kingDeltas[8][2]   = { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1} };
static const int bishopDirs[4][2]   = { {1,1},{1,-1},{-1,1},{-1,-1} };
static const int rookDirs[4][2]     = { {1,0},{-1,0},{0,1},{0,-1} };

static bool isSquareAttacked(const Board& b, int sq, int bySide) {
    int f0 = fileOf(sq), r0 = rankOf(sq);
    int pawnRankDir = (bySide == COL_WHITE) ? -1 : 1; 
    for (int df = -1; df <= 1; df += 2) {
        int f = f0 + df, r = r0 + pawnRankDir;
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            uint8_t p = b.sq[sqOf(f, r)];
            if (!isEmpty(p) && pieceColor(p) == bySide && pieceType(p) == PT_PAWN) return true;
        }
    }
    for (int i = 0; i < 8; ++i) {
        int f = f0 + knightDeltas[i][0], r = r0 + knightDeltas[i][1];
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            uint8_t p = b.sq[sqOf(f, r)];
            if (!isEmpty(p) && pieceColor(p) == bySide && pieceType(p) == PT_KNIGHT) return true;
        }
    }
    for (int i = 0; i < 8; ++i) {
        int f = f0 + kingDeltas[i][0], r = r0 + kingDeltas[i][1];
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            uint8_t p = b.sq[sqOf(f, r)];
            if (!isEmpty(p) && pieceColor(p) == bySide && pieceType(p) == PT_KING) return true;
        }
    }
    for (int d = 0; d < 4; ++d) {
        int f = f0 + bishopDirs[d][0], r = r0 + bishopDirs[d][1];
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            uint8_t p = b.sq[sqOf(f, r)];
            if (!isEmpty(p)) {
                if (pieceColor(p) == bySide && (pieceType(p) == PT_BISHOP || pieceType(p) == PT_QUEEN)) return true;
                break;
            }
            f += bishopDirs[d][0]; r += bishopDirs[d][1];
        }
    }
    for (int d = 0; d < 4; ++d) {
        int f = f0 + rookDirs[d][0], r = r0 + rookDirs[d][1];
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            uint8_t p = b.sq[sqOf(f, r)];
            if (!isEmpty(p)) {
                if (pieceColor(p) == bySide && (pieceType(p) == PT_ROOK || pieceType(p) == PT_QUEEN)) return true;
                break;
            }
            f += rookDirs[d][0]; r += rookDirs[d][1];
        }
    }
    return false;
}

static inline bool inCheck(const Board& b, int side) {
    return isSquareAttacked(b, b.kingSq[side], 1 - side);
}

struct MoveList {
    Move moves[256];
    int count;
    inline void add(uint8_t from, uint8_t to, uint8_t promo, uint8_t flags) {
        moves[count].from = from; moves[count].to = to;
        moves[count].promo = promo; moves[count].flags = flags;
        count++;
    }
};

static void genPseudoMoves(const Board& b, MoveList& ml) {
    ml.count = 0;
    int side = b.sideToMove;
    for (int sq = 0; sq < 64; ++sq) {
        uint8_t p = b.sq[sq];
        if (isEmpty(p) || pieceColor(p) != side) continue;
        int t = pieceType(p);
        int f0 = fileOf(sq), r0 = rankOf(sq);

        if (t == PT_PAWN) {
            int dir = (side == COL_WHITE) ? 1 : -1;
            int startRank = (side == COL_WHITE) ? 1 : 6;
            int promoRank = (side == COL_WHITE) ? 7 : 0;
            int r1 = r0 + dir;
            if (r1 >= 0 && r1 < 8 && isEmpty(b.sq[sqOf(f0, r1)])) {
                if (r1 == promoRank) {
                    ml.add(sq, sqOf(f0, r1), PT_QUEEN, 0);
                    ml.add(sq, sqOf(f0, r1), PT_ROOK, 0);
                    ml.add(sq, sqOf(f0, r1), PT_BISHOP, 0);
                    ml.add(sq, sqOf(f0, r1), PT_KNIGHT, 0);
                } else {
                    ml.add(sq, sqOf(f0, r1), 0, 0);
                    int r2 = r0 + 2 * dir;
                    if (r0 == startRank && isEmpty(b.sq[sqOf(f0, r2)])) {
                        ml.add(sq, sqOf(f0, r2), 0, MFLAG_DOUBLE);
                    }
                }
            }
            for (int df = -1; df <= 1; df += 2) {
                int f1 = f0 + df;
                if (f1 < 0 || f1 >= 8 || r1 < 0 || r1 >= 8) continue;
                int toSq = sqOf(f1, r1);
                uint8_t target = b.sq[toSq];
                if (!isEmpty(target) && pieceColor(target) != side) {
                    if (r1 == promoRank) {
                        ml.add(sq, toSq, PT_QUEEN, MFLAG_CAPTURE);
                        ml.add(sq, toSq, PT_ROOK, MFLAG_CAPTURE);
                        ml.add(sq, toSq, PT_BISHOP, MFLAG_CAPTURE);
                        ml.add(sq, toSq, PT_KNIGHT, MFLAG_CAPTURE);
                    } else {
                        ml.add(sq, toSq, 0, MFLAG_CAPTURE);
                    }
                } else if (b.epSquare == toSq) {
                    ml.add(sq, toSq, 0, MFLAG_CAPTURE | MFLAG_EP);
                }
            }
        } else if (t == PT_KNIGHT) {
            for (int i = 0; i < 8; ++i) {
                int f = f0 + knightDeltas[i][0], r = r0 + knightDeltas[i][1];
                if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;
                int toSq = sqOf(f, r);
                uint8_t target = b.sq[toSq];
                if (isEmpty(target)) ml.add(sq, toSq, 0, 0);
                else if (pieceColor(target) != side) ml.add(sq, toSq, 0, MFLAG_CAPTURE);
            }
        } else if (t == PT_KING) {
            for (int i = 0; i < 8; ++i) {
                int f = f0 + kingDeltas[i][0], r = r0 + kingDeltas[i][1];
                if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;
                int toSq = sqOf(f, r);
                uint8_t target = b.sq[toSq];
                if (isEmpty(target)) ml.add(sq, toSq, 0, 0);
                else if (pieceColor(target) != side) ml.add(sq, toSq, 0, MFLAG_CAPTURE);
            }
            int homeRank = (side == COL_WHITE) ? 0 : 7;
            if (r0 == homeRank && f0 == 4 && !inCheck(b, side)) {
                bool kRight = (side == COL_WHITE) ? (b.castleRights & CR_WK) : (b.castleRights & CR_BK);
                bool qRight = (side == COL_WHITE) ? (b.castleRights & CR_WQ) : (b.castleRights & CR_BQ);
                if (kRight && isEmpty(b.sq[sqOf(5, homeRank)]) && isEmpty(b.sq[sqOf(6, homeRank)])) {
                    if (!isSquareAttacked(b, sqOf(5, homeRank), 1 - side) &&
                        !isSquareAttacked(b, sqOf(6, homeRank), 1 - side)) {
                        ml.add(sq, sqOf(6, homeRank), 0, MFLAG_CASTLE_K);
                    }
                }
                if (qRight && isEmpty(b.sq[sqOf(3, homeRank)]) && isEmpty(b.sq[sqOf(2, homeRank)]) && isEmpty(b.sq[sqOf(1, homeRank)])) {
                    if (!isSquareAttacked(b, sqOf(3, homeRank), 1 - side) &&
                        !isSquareAttacked(b, sqOf(2, homeRank), 1 - side)) {
                        ml.add(sq, sqOf(2, homeRank), 0, MFLAG_CASTLE_Q);
                    }
                }
            }
        } else if (t == PT_BISHOP || t == PT_ROOK || t == PT_QUEEN) {
            const int(*dirs)[2] = nullptr;
            int ndirs = 0;
            if (t == PT_BISHOP) { dirs = bishopDirs; ndirs = 4; }
            else if (t == PT_ROOK) { dirs = rookDirs; ndirs = 4; }
            else { 
                for (int d = 0; d < 4; ++d) {
                    int f = f0, r = r0;
                    for (;;) {
                        f += bishopDirs[d][0]; r += bishopDirs[d][1];
                        if (f < 0 || f >= 8 || r < 0 || r >= 8) break;
                        int toSq = sqOf(f, r);
                        uint8_t target = b.sq[toSq];
                        if (isEmpty(target)) { ml.add(sq, toSq, 0, 0); continue; }
                        if (pieceColor(target) != side) ml.add(sq, toSq, 0, MFLAG_CAPTURE);
                        break;
                    }
                }
                for (int d = 0; d < 4; ++d) {
                    int f = f0, r = r0;
                    for (;;) {
                        f += rookDirs[d][0]; r += rookDirs[d][1];
                        if (f < 0 || f >= 8 || r < 0 || r >= 8) break;
                        int toSq = sqOf(f, r);
                        uint8_t target = b.sq[toSq];
                        if (isEmpty(target)) { ml.add(sq, toSq, 0, 0); continue; }
                        if (pieceColor(target) != side) ml.add(sq, toSq, 0, MFLAG_CAPTURE);
                        break;
                    }
                }
                continue;
            }
            for (int d = 0; d < ndirs; ++d) {
                int f = f0, r = r0;
                for (;;) {
                    f += dirs[d][0]; r += dirs[d][1];
                    if (f < 0 || f >= 8 || r < 0 || r >= 8) break;
                    int toSq = sqOf(f, r);
                    uint8_t target = b.sq[toSq];
                    if (isEmpty(target)) { ml.add(sq, toSq, 0, 0); continue; }
                    if (pieceColor(target) != side) ml.add(sq, toSq, 0, MFLAG_CAPTURE);
                    break;
                }
            }
        }
    }
}

static void makeMove(Board& b, const Move& m, Undo& u) {
    u.move = m;
    u.captured = b.sq[m.to];
    u.epSquareBefore = (uint8_t)(b.epSquare < 0 ? 64 : b.epSquare);
    u.castleRightsBefore = b.castleRights;
    u.halfmoveBefore = (uint8_t)b.halfmoveClock;

    int side = b.sideToMove;
    uint8_t moving = b.sq[m.from];
    int mtype = pieceType(moving);

    if (m.flags & MFLAG_EP) {
        int capSq = (side == COL_WHITE) ? m.to - 8 : m.to + 8;
        b.sq[capSq] = EMPTY;
    }

    b.sq[m.to] = moving;
    b.sq[m.from] = EMPTY;

    if (m.promo) {
        b.sq[m.to] = makePiece(side, m.promo);
    }

    if (mtype == PT_KING) {
        b.kingSq[side] = m.to;
        if (m.flags & MFLAG_CASTLE_K) {
            int homeRank = rankOf(m.from);
            b.sq[sqOf(5, homeRank)] = b.sq[sqOf(7, homeRank)];
            b.sq[sqOf(7, homeRank)] = EMPTY;
        } else if (m.flags & MFLAG_CASTLE_Q) {
            int homeRank = rankOf(m.from);
            b.sq[sqOf(3, homeRank)] = b.sq[sqOf(0, homeRank)];
            b.sq[sqOf(0, homeRank)] = EMPTY;
        }
        if (side == COL_WHITE) b.castleRights &= ~(CR_WK | CR_WQ);
        else b.castleRights &= ~(CR_BK | CR_BQ);
    }
    if (m.from == sqOf(0,0) || m.to == sqOf(0,0)) b.castleRights &= ~CR_WQ;
    if (m.from == sqOf(7,0) || m.to == sqOf(7,0)) b.castleRights &= ~CR_WK;
    if (m.from == sqOf(0,7) || m.to == sqOf(0,7)) b.castleRights &= ~CR_BQ;
    if (m.from == sqOf(7,7) || m.to == sqOf(7,7)) b.castleRights &= ~CR_BK;

    if (m.flags & MFLAG_DOUBLE) {
        b.epSquare = (side == COL_WHITE) ? m.from + 8 : m.from - 8;
    } else {
        b.epSquare = -1;
    }

    if (mtype == PT_PAWN || (m.flags & MFLAG_CAPTURE)) b.halfmoveClock = 0;
    else b.halfmoveClock++;

    b.sideToMove = 1 - side;
}

static void unmakeMove(Board& b, const Undo& u) {
    int side = 1 - b.sideToMove;
    b.sideToMove = side;
    const Move& m = u.move;
    uint8_t moved = b.sq[m.to];

    if (m.promo) {
        moved = makePiece(side, PT_PAWN);
    }
    b.sq[m.from] = moved;
    b.sq[m.to] = u.captured;

    if (m.flags & MFLAG_EP) {
        int capSq = (side == COL_WHITE) ? m.to - 8 : m.to + 8;
        b.sq[m.to] = EMPTY;
        b.sq[capSq] = makePiece(1 - side, PT_PAWN);
    }

    if (pieceType(moved) == PT_KING) {
        b.kingSq[side] = m.from;
        if (m.flags & MFLAG_CASTLE_K) {
            int homeRank = rankOf(m.from);
            b.sq[sqOf(7, homeRank)] = b.sq[sqOf(5, homeRank)];
            b.sq[sqOf(5, homeRank)] = EMPTY;
        } else if (m.flags & MFLAG_CASTLE_Q) {
            int homeRank = rankOf(m.from);
            b.sq[sqOf(0, homeRank)] = b.sq[sqOf(3, homeRank)];
            b.sq[sqOf(3, homeRank)] = EMPTY;
        }
    }

    b.castleRights = u.castleRightsBefore;
    b.epSquare = (u.epSquareBefore == 64) ? -1 : u.epSquareBefore;
    b.halfmoveClock = u.halfmoveBefore;
}

static void genLegalMoves(Board& b, MoveList& out) {
    MoveList pseudo;
    genPseudoMoves(b, pseudo);
    out.count = 0;
    int side = b.sideToMove;
    for (int i = 0; i < pseudo.count; ++i) {
        Undo u;
        makeMove(b, pseudo.moves[i], u);
        if (!isSquareAttacked(b, b.kingSq[side], 1 - side)) {
            out.moves[out.count++] = pseudo.moves[i];
        }
        unmakeMove(b, u);
    }
}

// ============================================================================
//  EVALUATION
// ============================================================================

static const int pieceValue[7] = { 0, 100, 320, 330, 500, 900, 20000 };

static const int pstPawn[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0,
};
static const int pstKnight[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};
static const int pstBishop[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};
static const int pstRook[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};
static const int pstQueen[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};
static const int pstKingMid[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};
static const int pstKingEnd[64] = {
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -50,-40,-30,-20,-20,-30,-40,-50,
};

static inline int flipSq(int sq) { return sqOf(fileOf(sq), 7 - rankOf(sq)); }

static int countMajors(const Board& b) {
    int n = 0;
    for (int i = 0; i < 64; ++i) {
        uint8_t p = b.sq[i];
        if (!isEmpty(p)) {
            int t = pieceType(p);
            if (t == PT_QUEEN || t == PT_ROOK || t == PT_BISHOP || t == PT_KNIGHT) n++;
        }
    }
    return n;
}

static int evaluate(const Board& b) {
    int score = 0;
    bool endgame = countMajors(b) <= 6;
    for (int sq = 0; sq < 64; ++sq) {
        uint8_t p = b.sq[sq];
        if (isEmpty(p)) continue;
        int t = pieceType(p);
        int c = pieceColor(p);
        int val = pieceValue[t];
        int pstIdx = (c == COL_WHITE) ? sq : flipSq(sq);
        int pst = 0;
        switch (t) {
            case PT_PAWN:   pst = pstPawn[pstIdx]; break;
            case PT_KNIGHT: pst = pstKnight[pstIdx]; break;
            case PT_BISHOP: pst = pstBishop[pstIdx]; break;
            case PT_ROOK:   pst = pstRook[pstIdx]; break;
            case PT_QUEEN:  pst = pstQueen[pstIdx]; break;
            case PT_KING:   pst = endgame ? pstKingEnd[pstIdx] : pstKingMid[pstIdx]; break;
        }
        int contrib = val + pst;
        score += (c == COL_WHITE) ? contrib : -contrib;
    }
    return score;
}

// ============================================================================
//  SEARCH
// ============================================================================

#define TT_SIZE (1 << 16)
enum { TT_EXACT, TT_LOWER, TT_UPPER };
struct TTEntry {
    uint64_t key;
    int16_t score;
    int8_t depth;
    uint8_t flag;
    Move best;
    bool used;
};
static TTEntry* tt = nullptr;

// Global array to store Killer Moves (up to max ply depth of 64)
static Move killerMoves[64][2];

static uint64_t zobristPieces[64][16];
static uint64_t zobristSide;
static uint64_t zobristCastle[16];
static uint64_t zobristEP[8];
static uint64_t rngState = 0x9E3779B97F4A7C15ULL;

static uint64_t xorshift64() {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 7;
    rngState ^= rngState << 17;
    return rngState;
}

static void initZobrist() {
    for (int s = 0; s < 64; ++s)
        for (int p = 0; p < 16; ++p)
            zobristPieces[s][p] = xorshift64();
    zobristSide = xorshift64();
    for (int i = 0; i < 16; ++i) zobristCastle[i] = xorshift64();
    for (int i = 0; i < 8; ++i) zobristEP[i] = xorshift64();
}

static uint64_t computeHash(const Board& b) {
    uint64_t h = 0;
    for (int s = 0; s < 64; ++s) {
        uint8_t p = b.sq[s];
        if (!isEmpty(p)) h ^= zobristPieces[s][p];
    }
    if (b.sideToMove == COL_BLACK) h ^= zobristSide;
    h ^= zobristCastle[b.castleRights & 0xF];
    if (b.epSquare >= 0) h ^= zobristEP[fileOf(b.epSquare)];
    return h;
}

static int nodesSearched = 0;
static const int MATE_SCORE = 30000;

static inline int moveScore(const Board& b, const Move& m, const Move& ttMove, bool hasTT, int ply) {
    // 1. Principal Variation / Transposition Table Move
    if (hasTT && m.from == ttMove.from && m.to == ttMove.to && m.promo == ttMove.promo) return 1000000;
    
    int s = 0;
    if (m.flags & MFLAG_CAPTURE) {
        // 2. Good Captures (MVV-LVA)
        uint8_t victim = b.sq[m.to];
        int victimType = (m.flags & MFLAG_EP) ? PT_PAWN : (isEmpty(victim) ? 0 : pieceType(victim));
        uint8_t attacker = b.sq[m.from];
        int attackerType = pieceType(attacker);
        s += 100000 + pieceValue[victimType] * 16 - pieceValue[attackerType];
    } else if (ply < 64) {
        // 3. Killer Moves (non-captures that caused recent branch alpha cuts)
        if (m.from == killerMoves[ply][0].from && m.to == killerMoves[ply][0].to && m.promo == killerMoves[ply][0].promo) return 90000;
        if (m.from == killerMoves[ply][1].from && m.to == killerMoves[ply][1].to && m.promo == killerMoves[ply][1].promo) return 80000;
    }
    
    if (m.promo == PT_QUEEN) s += 90000;
    return s;
}

static void sortMoves(Board& b, MoveList& ml, const Move& ttMove, bool hasTT, int ply) {
    int scores[256];
    for (int i = 0; i < ml.count; ++i) scores[i] = moveScore(b, ml.moves[i], ttMove, hasTT, ply);
    
    // Insertion sort: efficient for small hardware pipelines with short lists
    for (int i = 1; i < ml.count; ++i) {
        Move mv = ml.moves[i];
        int sc = scores[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < sc) {
            ml.moves[j + 1] = ml.moves[j];
            scores[j + 1] = scores[j];
            --j;
        }
        ml.moves[j + 1] = mv;
        scores[j + 1] = sc;
    }
}

static int quiescence(Board& b, int alpha, int beta, int side, int ply) {
    nodesSearched++;
    int standPat = evaluate(b) * (side == COL_WHITE ? 1 : -1);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    MoveList ml;
    genPseudoMoves(b, ml); // Changed to PseudoMoves for optimal sorting speedup[cite: 1]
    Move dummy{}; 
    sortMoves(b, ml, dummy, false, ply);
    
    for (int i = 0; i < ml.count; ++i) {
        if (!(ml.moves[i].flags & MFLAG_CAPTURE)) continue;
        
        Undo u;
        makeMove(b, ml.moves[i], u);
        
        // Lazy Legality Check[cite: 1]
        if (isSquareAttacked(b, b.kingSq[side], 1 - side)) {
            unmakeMove(b, u);
            continue; 
        }
        
        int score = -quiescence(b, -beta, -alpha, 1 - side, ply + 1);
        unmakeMove(b, u);
        
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

static int negamax(Board& b, int depth, int alpha, int beta, int side, Move* outBest, int ply) {
    nodesSearched++;
    uint64_t hash = computeHash(b);
    uint32_t idx = (uint32_t)(hash & (TT_SIZE - 1));
    Move ttMove{}; bool hasTT = false;
    int origAlpha = alpha;

    if (tt[idx].used && tt[idx].key == hash && tt[idx].depth >= depth) {
        hasTT = true;
        ttMove = tt[idx].best;
        if (tt[idx].flag == TT_EXACT) { if (outBest) *outBest = ttMove; return tt[idx].score; }
        else if (tt[idx].flag == TT_LOWER) { if (tt[idx].score > alpha) alpha = tt[idx].score; }
        else if (tt[idx].flag == TT_UPPER) { if (tt[idx].score < beta) beta = tt[idx].score; }
        if (alpha >= beta) return tt[idx].score;
    } else if (tt[idx].used && tt[idx].key == hash) {
        hasTT = true;
        ttMove = tt[idx].best;
    }

    if (depth <= 0) {
        return quiescence(b, alpha, beta, side, ply);
    }

    // NULL MOVE PRUNING: Avoid searching bad trees if passing a turn doesn't break beta
    if (depth >= 3 && !inCheck(b, side) && ply > 0) {
        int ep_cache = b.epSquare;
        b.epSquare = -1;
        b.sideToMove = 1 - side;
        
        int nullScore = -negamax(b, depth - 3, -beta, -beta + 1, 1 - side, nullptr, ply + 1);
        
        b.sideToMove = side;
        b.epSquare = ep_cache;
        
        if (nullScore >= beta) return beta;
    }

    MoveList ml;
    genPseudoMoves(b, ml); // Core improvement: Use pseudo moves during search iterations[cite: 1]

    sortMoves(b, ml, ttMove, hasTT, ply);

    int best = -MATE_SCORE * 2;
    Move bestMove = ml.count > 0 ? ml.moves[0] : Move{};
    int legalMovesFound = 0;

    for (int i = 0; i < ml.count; ++i) {
        Undo u;
        makeMove(b, ml.moves[i], u);
        
        // Lazy Legality Check[cite: 1]
        if (isSquareAttacked(b, b.kingSq[side], 1 - side)) {
            unmakeMove(b, u);
            continue; 
        }
        legalMovesFound++;

        int score = -negamax(b, depth - 1, -beta, -alpha, 1 - side, nullptr, ply + 1);
        unmakeMove(b, u);

        if (score > best) {
            best = score;
            bestMove = ml.moves[i];
        }
        if (best > alpha) alpha = best;
        
        // Beta Cutoff
        if (alpha >= beta) {
            // Log Killer Move if the cutoff wasn't a tactical capture[cite: 1]
            if (!(ml.moves[i].flags & MFLAG_CAPTURE) && ply < 64) {
                killerMoves[ply][1] = killerMoves[ply][0];
                killerMoves[ply][0] = ml.moves[i];
            }
            break; 
        }
    }

    // Native Checkmate and Stalemate resolution inside pseudo loop
    if (legalMovesFound == 0) {
        if (inCheck(b, side)) return -MATE_SCORE + ply; 
        return 0; 
    }

    uint8_t flag;
    if (best <= origAlpha) flag = TT_UPPER;
    else if (best >= beta) flag = TT_LOWER;
    else flag = TT_EXACT;
    
    tt[idx].used = true;
    tt[idx].key = hash;
    tt[idx].score = (int16_t)best;
    tt[idx].depth = (int8_t)depth;
    tt[idx].flag = flag;
    tt[idx].best = bestMove;

    if (outBest) *outBest = bestMove;
    return best;
}

// Iterative deepening driver with UI updates
static void drawCenteredText(int cy, const char* s, uint16_t col, int scale);

static Move searchBestMove(Board& b, int maxDepth) {
    int side = b.sideToMove;
    Move best;
    MoveList ml;
    genLegalMoves(b, ml);
    if (ml.count > 0) best = ml.moves[0];
    nodesSearched = 0;

    // Reset Killer moves before starting a root-node cycle
    Mem_Memset(killerMoves, 0, sizeof(killerMoves));

    int lastScore = evaluate(b) * (side == COL_WHITE ? 1 : -1);

    for (int d = 1; d <= maxDepth; ++d) {
        // Draw progress bar at the top
        int progressW = (d * width) / maxDepth;
        fillRect(0, 0, progressW, 10, COLOR_PROGRESS);
        fillRect(progressW, 0, width - progressW, 10, COLOR_BG);
        
        // Clear area underneath progress bar for text
        fillRect(0, 10, width, 20, COLOR_BG);

        // Build string: "Depth: <d> Eval: <score>"
        char info[64];
        int idx = 0;
        
        const char* d_str = "Depth: ";
        while (*d_str) info[idx++] = *d_str++;
        info[idx++] = '0' + d;
        
        const char* e_str = " Eval: ";
        while (*e_str) info[idx++] = *e_str++;
        
        int scoreWhite = (side == COL_WHITE) ? lastScore : -lastScore;
        if (scoreWhite < 0) {
            info[idx++] = '-';
            scoreWhite = -scoreWhite;
        } else {
            info[idx++] = '+';
        }
        
        int w = scoreWhite / 100;
        int f = scoreWhite % 100;
        char wbuf[10]; int wlen = 0;
        if (w == 0) wbuf[wlen++] = '0';
        while (w > 0) { wbuf[wlen++] = '0' + (w % 10); w /= 10; }
        for (int i = wlen - 1; i >= 0; --i) info[idx++] = wbuf[i];
        
        info[idx++] = '.';
        info[idx++] = '0' + (f / 10);
        info[idx++] = '0' + (f % 10);
        info[idx++] = '\0';
        
        drawCenteredText(12, info, COLOR_TEXT, 1);
        LCD_Refresh();

        Move iterBest;
        // Modified with explicit ply parameter tracking (0 at search root node)[cite: 1]
        int currentScore = negamax(b, d, -MATE_SCORE * 2, MATE_SCORE * 2, side, &iterBest, 0);
        lastScore = currentScore;
        best = iterBest;
    }
    
    // Clear top display once done
    fillRect(0, 0, width, 30, COLOR_BG);
    LCD_Refresh();
    return best;
}

// ============================================================================
//  UI / APP STATE
// ============================================================================

enum AppScreen { SCREEN_MENU, SCREEN_GAME, SCREEN_GAMEOVER };
static AppScreen screen = SCREEN_MENU;

static int playerSide = COL_WHITE;
static int botDepth = 3;

static int boardOriginX = 0;
static int boardOriginY = 104; // Set in main to exact center
static int squareSize = 40; 

static int selectedSq = -1;
static MoveList legalForSelected;
static int lastMoveFrom = -1, lastMoveTo = -1;
static bool gameOver = false;
static int gameOverResult = 0; 

static Button btnToggleSide = { 40, 140, 240, 50, "Side: White" };
static Button btnDepthMinus = { 40, 255, 60, 50, "-" };
static Button btnDepthPlus  = { 220, 255, 60, 50, "+" };
static Button btnStart      = { 60, 340, 200, 60, "Start" };
static Button btnReset      = { 60, 456, 200, 40, "Reset" };

static void updateMenuLabels() {
    static char sideLabel[16];
    const char* txt = (playerSide == COL_WHITE) ? "Side: White" : "Side: Black";
    int i = 0;
    while (txt[i] && i < 15) { sideLabel[i] = txt[i]; ++i; }
    sideLabel[i] = 0;
    btnToggleSide.label = sideLabel;
}

static void drawCenteredText(int cy, const char* s, uint16_t col, int scale) {
    int tw = textWidth(s, scale);
    int tx = (width - tw) / 2;
    drawText(tx, cy, s, col, scale);
}

static void drawMenu() {
    fillScreen(COLOR_BG);
    drawCenteredText(30, "Chess", COLOR_TEXT, 3);
    drawCenteredText(80, "Made by DeceasedSnake82", COLOR_TEXT, 2);

    updateMenuLabels();
    btnToggleSide.draw(false);

    drawCenteredText(230, "Bot Depth", COLOR_TEXT, 2);
    btnDepthMinus.draw(false);
    btnDepthPlus.draw(false);

    char depthStr[8];
    depthStr[0] = (char)('0' + botDepth);
    depthStr[1] = 0;
    int tw = textWidth(depthStr, 3);
    drawText(160 - tw / 2, 265, depthStr, COLOR_TEXT, 3);

    btnStart.draw(false);

    LCD_Refresh();
}

static inline int screenXtoFile(int x, bool playerIsWhite) {
    int col = (x - boardOriginX) / squareSize;
    return playerIsWhite ? col : (7 - col);
}

static inline int screenYtoRank(int y, bool playerIsWhite) {
    int row = (y - boardOriginY) / squareSize;
    return playerIsWhite ? (7 - row) : row;
}

static void drawSprite16(int px, int py, const uint16_t* sprite, uint16_t fg, uint16_t outline, int scale) {
    for (int pass = 0; pass < 2; ++pass) {
        uint16_t drawColor = (pass == 0) ? outline : fg;
        for (int row = 0; row < 16; ++row) {
            uint16_t bits = sprite[row];
            for (int colb = 0; colb < 16; ++colb) {
                if (bits & (1 << (15 - colb))) {
                    if (pass == 0) {
                        int ox = px + colb * scale;
                        int oy = py + row * scale;
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                for (int sy = 0; sy < scale; ++sy)
                                    for (int sx = 0; sx < scale; ++sx)
                                        setPixel(ox + dx + sx, oy + dy + sy, drawColor);
                            }
                        }
                    }
                }
            }
        }
        if (pass == 1) {
            for (int row = 0; row < 16; ++row) {
                uint16_t bits = sprite[row];
                for (int colb = 0; colb < 16; ++colb) {
                    if (bits & (1 << (15 - colb))) {
                        int ox = px + colb * scale;
                        int oy = py + row * scale;
                        for (int sy = 0; sy < scale; ++sy)
                            for (int sx = 0; sx < scale; ++sx)
                                setPixel(ox + sx, oy + sy, drawColor);
                    }
                }
            }
        }
    }
}

static bool squareIsLegalDest(int sq) {
    for (int i = 0; i < legalForSelected.count; ++i)
        if (legalForSelected.moves[i].to == sq) return true;
    return false;
}

static void drawBoard() {
    fillScreen(COLOR_BG);
    bool playerIsWhite = (playerSide == COL_WHITE);

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int file = playerIsWhite ? col : (7 - col);
            int rank = playerIsWhite ? (7 - row) : row;
            int sq = sqOf(file, rank);
            int sx = boardOriginX + col * squareSize;
            int sy = boardOriginY + row * squareSize;
            bool isLight = ((file + rank) % 2 == 1);
            uint16_t sqColor = isLight ? COLOR_LIGHT_SQ : COLOR_DARK_SQ;
            fillRect(sx, sy, squareSize, squareSize, sqColor);

            if (sq == lastMoveFrom || sq == lastMoveTo) {
                drawRectOutline(sx, sy, squareSize, squareSize, COLOR_LASTMOVE, 2);
            }
            if (sq == selectedSq) {
                drawRectOutline(sx, sy, squareSize, squareSize, COLOR_SELECT, 3);
            }

            uint8_t p = board.sq[sq];
            if (!isEmpty(p)) {
                int t = pieceType(p);
                int c = pieceColor(p);
                const uint16_t* spr = kPieceSprites[t - 1];
                uint16_t fg = (c == COL_WHITE) ? COLOR_WHITE_PC : COLOR_BLACK_PC;
                uint16_t oline = (c == COL_WHITE) ? COLOR_OUTLINE_W : COLOR_OUTLINE_B;
                int scale = squareSize / 16; 
                if (scale < 1) scale = 1;
                int spriteW = 16 * scale;
                int px = sx + (squareSize - spriteW) / 2;
                int py = sy + (squareSize - spriteW) / 2;
                drawSprite16(px, py, spr, fg, oline, scale);
            }

            if (!isEmpty(p) && pieceType(p) == PT_KING && inCheck(board, pieceColor(p))) {
                drawRectOutline(sx + 1, sy + 1, squareSize - 2, squareSize - 2, COLOR_CHECK, 2);
            }

            if (selectedSq >= 0 && squareIsLegalDest(sq)) {
                bool isCapture = !isEmpty(p) || (p == EMPTY && false);
                bool epCapture = false;
                for (int i = 0; i < legalForSelected.count; ++i) {
                    if (legalForSelected.moves[i].to == sq && (legalForSelected.moves[i].flags & MFLAG_EP)) epCapture = true;
                }
                uint16_t dotCol = (isCapture || epCapture) ? COLOR_CAPTUREDOT : COLOR_DOT;
                fillCircle(sx + squareSize / 2, sy + squareSize / 2, isCapture || epCapture ? squareSize / 3 : squareSize / 6, dotCol);
            }
        }
    }

    fillRect(0, boardOriginY + 8 * squareSize, width, height - (boardOriginY + 8 * squareSize), COLOR_BG);
    
    if (gameOver) {
        const char* statusMsg;
        if (gameOverResult == 1) statusMsg = "White wins!";
        else if (gameOverResult == -1) statusMsg = "Black wins!";
        else statusMsg = "Draw!";
        drawCenteredText(boardOriginY + 8 * squareSize + 12, statusMsg, COLOR_TEXT, 2);
    }

    btnReset.draw(false);
    LCD_Refresh();
}

static void checkGameOver() {
    MoveList ml;
    genLegalMoves(board, ml);
    if (ml.count == 0) {
        gameOver = true;
        if (inCheck(board, board.sideToMove)) {
            gameOverResult = (board.sideToMove == COL_WHITE) ? -1 : 1;
        } else {
            gameOverResult = 0;
        }
    } else if (board.halfmoveClock >= 100) {
        gameOver = true;
        gameOverResult = 0;
    }
}

static void doMove(const Move& m) {
    Undo u;
    makeMove(board, m, u);
    lastMoveFrom = m.from;
    lastMoveTo = m.to;
    selectedSq = -1;
    legalForSelected.count = 0;
    checkGameOver();
}

static void botMakeMove() {
    if (gameOver) return;
    Move m = searchBestMove(board, botDepth);
    doMove(m);
}

static void resetGame() {
    setupStartPosition(board);
    selectedSq = -1;
    legalForSelected.count = 0;
    lastMoveFrom = -1;
    lastMoveTo = -1;
    gameOver = false;
    gameOverResult = 0;
    Mem_Memset(tt, 0, sizeof(TTEntry) * TT_SIZE);
}

static void handleBoardTouch(int x, int y) {
    if (gameOver) return;
    if (board.sideToMove != playerSide) return;
    bool playerIsWhite = (playerSide == COL_WHITE);
    if (x < boardOriginX || x >= boardOriginX + 8 * squareSize) return;
    if (y < boardOriginY || y >= boardOriginY + 8 * squareSize) return;
    int file = screenXtoFile(x, playerIsWhite);
    int rank = screenYtoRank(y, playerIsWhite);
    int sq = sqOf(file, rank);

    if (selectedSq >= 0) {
        for (int i = 0; i < legalForSelected.count; ++i) {
            if (legalForSelected.moves[i].to == sq) {
                doMove(legalForSelected.moves[i]);
                return;
            }
        }
        uint8_t p = board.sq[sq];
        if (!isEmpty(p) && pieceColor(p) == playerSide) {
            selectedSq = sq;
            MoveList all;
            genLegalMoves(board, all);
            legalForSelected.count = 0;
            for (int i = 0; i < all.count; ++i)
                if (all.moves[i].from == sq) legalForSelected.moves[legalForSelected.count++] = all.moves[i];
        } else {
            selectedSq = -1;
            legalForSelected.count = 0;
        }
    } else {
        uint8_t p = board.sq[sq];
        if (!isEmpty(p) && pieceColor(p) == playerSide) {
            selectedSq = sq;
            MoveList all;
            genLegalMoves(board, all);
            legalForSelected.count = 0;
            for (int i = 0; i < all.count; ++i)
                if (all.moves[i].from == sq) legalForSelected.moves[legalForSelected.count++] = all.moves[i];
        }
    }
}

// ============================================================================
//  MAIN
// ============================================================================

int main() {
    tt = (TTEntry*)Mem_Malloc(sizeof(TTEntry) * TT_SIZE);
    Mem_Memset(tt, 0, sizeof(TTEntry) * TT_SIZE);
    initZobrist();
    setupStartPosition(board);
    
    boardOriginX = (width - 8 * squareSize) / 2;
    boardOriginY = (height - 8 * squareSize) / 2;

    screen = SCREEN_MENU;
    drawMenu();

    struct Input_Event event;
    bool running = true;

    while (running) {
        GetInput(&event, 0xFFFFFFFF, 0x10);

        if (event.type == EVENT_KEY) {
            if (event.data.key.direction == KEY_PRESSED &&
                event.data.key.keyCode == KEYCODE_POWER_CLEAR) {
                running = false;
            }
        } else if (event.type == EVENT_TOUCH) {
            if (event.data.touch_single.direction == TOUCH_DOWN) {
                int tx = event.data.touch_single.p1_x;
                int ty = event.data.touch_single.p1_y;

                if (screen == SCREEN_MENU) {
                    if (btnToggleSide.contains(tx, ty)) {
                        playerSide = 1 - playerSide;
                        drawMenu();
                    } else if (btnDepthMinus.contains(tx, ty)) {
                        if (botDepth > 1) botDepth--;
                        drawMenu();
                    } else if (btnDepthPlus.contains(tx, ty)) {
                        if (botDepth < 9) botDepth++;
                        drawMenu();
                    } else if (btnStart.contains(tx, ty)) {
                        resetGame();
                        screen = SCREEN_GAME;
                        drawBoard();
                        if (board.sideToMove != playerSide) {
                            botMakeMove();
                            drawBoard();
                        }
                    }
                } else { 
                    if (btnReset.contains(tx, ty)) {
                        screen = SCREEN_MENU;
                        drawMenu();
                    } else {
                        handleBoardTouch(tx, ty);
                        drawBoard();
                        if (!gameOver && screen == SCREEN_GAME && board.sideToMove != playerSide) {
                            botMakeMove();
                            drawBoard();
                        }
                    }
                }
            }
        }
    }

    Mem_Free(tt);
    return 0;
}
