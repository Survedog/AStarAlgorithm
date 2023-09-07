#pragma once

#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <queue>
#include <list>

#pragma comment(lib, "d2d1")

#define GRID_ROW_SQUARE_AMOUNT 10
#define GRID_COL_SQUARE_AMOUNT 10
#define MAX_SELECT_AMOUNT 2

typedef struct Position {
	float x, y;

	Position(float x, float y) : x(x), y(y) {}
};

typedef struct SquareId {
	int row;
	int col;

	bool operator==(const SquareId other) const {
		return (this->row == other.row && this->col == other.col);
	}

	inline bool IsNotAssigned() {
		return row == -1;
	}
} SquareId;

typedef struct GridSquare {
	SquareId id = { -1, -1 };
	SquareId parentId = { -1, -1 };
	D2D1_RECT_F rect = { 0.0f, 0.0f, 0.0f ,0.0f };
	BOOL isSelected = FALSE;
	BOOL isBlocked = FALSE;
	int g = 0, h = 0, f = 0;

	bool operator>(const GridSquare other) const {
		return this->f > other.f;
	}

	bool operator<(const GridSquare other) const {
		return this->f < other.f;
	}

	bool operator==(const GridSquare other) const {
		return (this->id == other.id);
	}
} GridSquare;

class AStarWindow
{
	HWND _hWnd;
	ID2D1Factory* _pFactory;
	ID2D1HwndRenderTarget* _pRenderTarget;
	ID2D1SolidColorBrush* _pGridColorBrush;
	ID2D1SolidColorBrush* _pSelectColorBrush;

	GridSquare _gridSquare[GRID_ROW_SQUARE_AMOUNT][GRID_COL_SQUARE_AMOUNT];
	Position _gridLeftTopPos = { 0.0f, 0.0f };
	const float _gridSquareEdgeLength = 50.0f;

	int _selectedSquareCount = 0;
	GridSquare *_pStartSquare = NULL, *_pDestSquare = NULL;
	std::list<SquareId> _path;

	void CreateGridData();
	HRESULT CreateGraphicsResources();
	void DiscardGraphicsResources();

	void CalculateAstarPath();
	void ClearGridSquaresParentData();

	void Draw();
	void DrawPath(ID2D1RenderTarget* _pRenderTarget, ID2D1SolidColorBrush* pBrush);
	void DrawBlockSignOnSquare(ID2D1RenderTarget* _pRenderTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F rect);
	void Resize();

	LRESULT CreateResourcesAndData();
	void OnDestroy();
	void OnMouseLButtonClick(LPARAM clickMessageLParam);
	void OnMouseMiddleButtonClick(LPARAM clickMessageLParam);

public:
	AStarWindow() : _hWnd(NULL), _pFactory(NULL), _pRenderTarget(NULL), _pGridColorBrush(NULL), _pSelectColorBrush(NULL) {}

	void SetHWnd(HWND hWnd) { _hWnd = hWnd; }
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	PCWSTR WindowClassName() const { return L"AStarWindow"; }
	PCWSTR WindowTitleName() const { return L"AStar"; }
};

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}