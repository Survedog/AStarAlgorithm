#include "AStarWindow.h"

void AStarWindow::CreateGridData()
{
	if (_pRenderTarget == NULL)
		return;
	
	D2D1_SIZE_F clientRectSize = _pRenderTarget->GetSize();
	_gridLeftTopPos.x = clientRectSize.width / 2 - _gridSquareEdgeLength * 5;
	_gridLeftTopPos.y = clientRectSize.height / 2 - _gridSquareEdgeLength * 5;

	for (int row = 0; row < GRID_ROW_SQUARE_AMOUNT; ++row)
	{
		Position squareLeftTopPos = { _gridLeftTopPos.x, _gridLeftTopPos.y + _gridSquareEdgeLength * row };

		for (int col = 0; col < GRID_COL_SQUARE_AMOUNT; ++col)
		{
			_gridSquare[row][col].id.row = row;
			_gridSquare[row][col].id.col = col;

			_gridSquare[row][col].rect.left = squareLeftTopPos.x;
			_gridSquare[row][col].rect.right = squareLeftTopPos.x + _gridSquareEdgeLength;
			_gridSquare[row][col].rect.top = squareLeftTopPos.y;
			_gridSquare[row][col].rect.bottom = squareLeftTopPos.y + _gridSquareEdgeLength;

			squareLeftTopPos.x += _gridSquareEdgeLength;
		}
	}
}

HRESULT AStarWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (_pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(_hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = _pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(_hWnd, size),
			&_pRenderTarget);

		if (SUCCEEDED(hr))
		{
			const D2D1_COLOR_F gridColor = D2D1::ColorF(D2D1::ColorF::Black);
			hr = _pRenderTarget->CreateSolidColorBrush(gridColor, &_pGridColorBrush);
			if (SUCCEEDED(hr))
			{
				const D2D1_COLOR_F selectColor = D2D1::ColorF(D2D1::ColorF::Yellow);
				hr = _pRenderTarget->CreateSolidColorBrush(selectColor, &_pSelectColorBrush);
			}
		}
	}
	return hr;
}

void AStarWindow::DiscardGraphicsResources()
{
	SafeRelease(&_pRenderTarget);
	SafeRelease(&_pGridColorBrush);
	SafeRelease(&_pSelectColorBrush);
}

void AStarWindow::Draw()
{
	if (_pRenderTarget == NULL)
		return;

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(_hWnd, &ps);

	_pRenderTarget->BeginDraw();
	_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	for (int row = 0; row < GRID_ROW_SQUARE_AMOUNT; ++row)
		for (int col = 0; col < GRID_COL_SQUARE_AMOUNT; ++col)
		{
			if (_gridSquare[row][col].isBlocked)
				DrawBlockSignOnSquare(_pRenderTarget, _pGridColorBrush, _gridSquare[row][col].rect);
			else if (_gridSquare[row][col].isSelected)
				_pRenderTarget->FillRectangle(_gridSquare[row][col].rect, _pSelectColorBrush);

			_pRenderTarget->DrawRectangle(_gridSquare[row][col].rect, _pGridColorBrush);
		}

	if (_selectedSquareCount == 2)
		DrawPath(_pRenderTarget, _pGridColorBrush);

	HRESULT hr = _pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
	{
		DiscardGraphicsResources();
		CreateGraphicsResources();
	}

	EndPaint(_hWnd, &ps);
}

void AStarWindow::DrawPath(ID2D1RenderTarget* _pRenderTarget, ID2D1SolidColorBrush* pBrush)
{
	if (_path.empty())
		return;

	auto cIterator = _path.cbegin();
	while (true) {
		GridSquare* lineStartSquare = &_gridSquare[cIterator->row][cIterator->col];
		if (++cIterator == _path.cend())
			break;
		GridSquare* lineEndSquare = &_gridSquare[cIterator->row][cIterator->col];

		D2D1_POINT_2F lineStartPoint = { lineStartSquare->rect.left + _gridSquareEdgeLength / 2, lineStartSquare->rect.top + _gridSquareEdgeLength / 2 };
		D2D1_POINT_2F lineEndPoint = { lineEndSquare->rect.left + _gridSquareEdgeLength / 2, lineEndSquare->rect.top + _gridSquareEdgeLength / 2 };
		_pRenderTarget->DrawLine(lineStartPoint, lineEndPoint, pBrush);
	}
}

void AStarWindow::DrawBlockSignOnSquare(ID2D1RenderTarget* _pRenderTarget, ID2D1SolidColorBrush* pBrush, D2D1_RECT_F rect)
{
	D2D1_POINT_2F lineStartPoint, lineEndPoint;

	lineStartPoint.x = rect.left;
	lineStartPoint.y = rect.top;
	lineEndPoint.x = rect.right;
	lineEndPoint.y = rect.bottom;
	_pRenderTarget->DrawLine(lineStartPoint, lineEndPoint, pBrush);

	lineStartPoint.x = rect.right;
	lineStartPoint.y = rect.top;
	lineEndPoint.x = rect.left;
	lineEndPoint.y = rect.bottom;
	_pRenderTarget->DrawLine(lineStartPoint, lineEndPoint, pBrush);
}

void AStarWindow::CalculateAstarPath()
{
	if (_pStartSquare == NULL || _pDestSquare == NULL)
		return;

	std::priority_queue<GridSquare, std::vector<GridSquare>, std::greater<GridSquare>> openList;
	std::vector<GridSquare> closeList;

	_path.clear();
	ClearGridSquaresParentData();
	openList.push(*_pStartSquare);

	GridSquare currentSquare = {};
	GridSquare* adjacentSquares[4];
	while (true) {
		currentSquare = openList.top();
		openList.pop();
		closeList.push_back(currentSquare);

		if (currentSquare.id == _pDestSquare->id)
			break;

		int currentRow = currentSquare.id.row;
		int currentCol = currentSquare.id.col;

		adjacentSquares[0] = currentRow - 1 >= 0 ? &_gridSquare[currentRow - 1][currentCol] : NULL;
		adjacentSquares[1] = currentCol - 1 >= 0 ? &_gridSquare[currentRow][currentCol - 1] : NULL;
		adjacentSquares[2] = &_gridSquare[currentRow + 1][currentCol];
		adjacentSquares[3] = &_gridSquare[currentRow][currentCol + 1];

		for (int i = 0; i < 4; i++) {
			if (adjacentSquares[i] == NULL || adjacentSquares[i]->isBlocked || std::find(closeList.cbegin(), closeList.cend(), *adjacentSquares[i]) != closeList.cend())
				continue;

			if (adjacentSquares[i]->parentId.IsNotAssigned()) {
				adjacentSquares[i]->parentId = currentSquare.id;
				adjacentSquares[i]->g = currentSquare.g + 1;
				adjacentSquares[i]->h = abs(adjacentSquares[i]->id.row - _pDestSquare->id.row) + abs(adjacentSquares[i]->id.col - _pDestSquare->id.col);
				adjacentSquares[i]->f = adjacentSquares[i]->g + adjacentSquares[i]->h;
				openList.push(*adjacentSquares[i]);
			}
		}

		if (openList.empty()) {
			MessageBox(_hWnd, L"Path doesn't exist.\n", L"WARNING", MB_ICONERROR | MB_OK);
			return;
		}
	}

	while (true) {
		_path.push_front(currentSquare.id);
		if (currentSquare.id == _pStartSquare->id)
			break;

		int parentRow = currentSquare.parentId.row;
		int parentCol = currentSquare.parentId.col;
		currentSquare = _gridSquare[parentRow][parentCol];
	}
}

void AStarWindow::ClearGridSquaresParentData() {
	for (int row = 0; row < GRID_ROW_SQUARE_AMOUNT; ++row)
		for (int col = 0; col < GRID_COL_SQUARE_AMOUNT; ++col) {
			_gridSquare[row][col].parentId.row = -1;
			_gridSquare[row][col].parentId.col = -1;
		}
}


void AStarWindow::OnMouseLButtonClick(LPARAM clickMessageLParam)
{
	int clickXPos = GET_X_LPARAM(clickMessageLParam);
	int clickYPos = GET_Y_LPARAM(clickMessageLParam);

	int row = (clickYPos - _gridLeftTopPos.y) / _gridSquareEdgeLength;
	int col = (clickXPos - _gridLeftTopPos.x) / _gridSquareEdgeLength;

	if (row < 0 || col < 0)
	{
		OutputDebugString(L"OnMouseLButtonClick - row or col value is negative.\n");
		return;
	}
	else if (_gridSquare[row][col].isBlocked)
	{
		OutputDebugString(L"OnMouseLButtonClick - selected square is blocked.\n");
		return;
	}

	if (_gridSquare[row][col].isSelected) // cancel selection
	{
		_gridSquare[row][col].isSelected = FALSE;
		if (_pStartSquare != NULL && _gridSquare[row][col] == *_pStartSquare)
			_pStartSquare = NULL;
		else
			_pDestSquare = NULL;
		_selectedSquareCount--;
	}
	else if (_selectedSquareCount < MAX_SELECT_AMOUNT) // select a square
	{
		_gridSquare[row][col].isSelected = TRUE;
		if (_pStartSquare == NULL)
			_pStartSquare = &_gridSquare[row][col];
		else
			_pDestSquare = &_gridSquare[row][col];
		_selectedSquareCount++;
	}
	else
	{
		OutputDebugString(L"OnMouseLButtonClick - reached select amount limit.\n");
		return;
	}
}

void AStarWindow::OnMouseMiddleButtonClick(LPARAM clickMessageLParam)
{
	int clickXPos = GET_X_LPARAM(clickMessageLParam);
	int clickYPos = GET_Y_LPARAM(clickMessageLParam);

	int row = (clickYPos - _gridLeftTopPos.y) / _gridSquareEdgeLength;
	int col = (clickXPos - _gridLeftTopPos.x) / _gridSquareEdgeLength;

	if (row < 0 || col < 0)
	{
		OutputDebugString(L"OnMouseMiddleButtonClick - row or col value is negative.\n");
		return;
	}
	else if (_gridSquare[row][col].isSelected)
	{
		OutputDebugString(L"OnMouseMiddleButtonClick - path start/destination block can't be blocked.\n");
		return;
	}

	if (_gridSquare[row][col].isBlocked)
		_gridSquare[row][col].isBlocked = FALSE;
	else
	{
		_gridSquare[row][col].isBlocked = TRUE;
		_gridSquare[row][col].isSelected = FALSE;
	}
}

HRESULT AStarWindow::CreateResourcesAndData()
{
	HRESULT hr = S_OK;
	
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pFactory);
	if (SUCCEEDED(hr))
	{		
		hr = CreateGraphicsResources();
		if (SUCCEEDED(hr))
			CreateGridData();
	}

	return hr;
}

void AStarWindow::OnDestroy()
{
	DiscardGraphicsResources();
	SafeRelease(&_pFactory);
}

void AStarWindow::Resize()
{
	if (_pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(_hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		_pRenderTarget->Resize(size);
		CreateGridData();
		InvalidateRect(_hWnd, NULL, FALSE);
	}
}

LRESULT AStarWindow::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		SetHWnd(hWnd);
		if (FAILED(CreateResourcesAndData()))
			return -1;
		return 0;

	case WM_DESTROY:
		OnDestroy();
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
		Draw();
		return 0;

	case WM_LBUTTONUP:
		OnMouseLButtonClick(lParam);
		if (_selectedSquareCount == 2) CalculateAstarPath();
		else _path.clear();
		Draw();
		return 0;

	case WM_MBUTTONUP:
		OnMouseMiddleButtonClick(lParam);
		if (_selectedSquareCount == 2)
			CalculateAstarPath();		
		Draw();
		return 0;

	case WM_SIZE:
		Resize();
		Draw();
		return 0;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}