#pragma once

class ChessboardPosition
{
public:
    ChessboardPosition(int panCenter, int tiltCenter)
    {
        this->panCenter = panCenter;
        this->tiltCenter = tiltCenter;
    }
    ChessboardPosition(ChessboardPosition& other)
    {
        this->panCenter = other.panCenter;
        this->tiltCenter = other.tiltCenter;
    }
    int panCenter;
    int tiltCenter;
};