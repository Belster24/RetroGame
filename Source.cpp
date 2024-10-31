#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace std;

#include <Windows.h>

int nScreenWidth = 120;
int nScreenHeight = 30;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerLook = 0.0f;

int nMapHeight = 16;
int nMapWidth = 16;

float FOV = 3.14159 / 4.0;
float Depth = 16.0f;

int main() {

	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"########.......#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#........####..#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();


	//Game Loop
	while (1) 
	{
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();


		//Controls
		//Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000) fPlayerLook -= (0.8f) * fElapsedTime;
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000) fPlayerLook += (0.8f) * fElapsedTime;
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			fPlayerX += sinf(fPlayerLook) * 5.0f * fElapsedTime;
			fPlayerX += cosf(fPlayerLook) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerLook) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerLook) * 5.0f * fElapsedTime;
			}
		}
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerLook) * 5.0f * fElapsedTime;
			fPlayerX -= cosf(fPlayerLook) * 5.0f * fElapsedTime;
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerLook) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerLook) * 5.0f * fElapsedTime;
			}
		}


		for (int x = 0; x < nScreenWidth; x++) {
		
			//for each column, claculate the projected ray angle into world space
			float fRayAngle = (fPlayerLook - FOV / 2.0f) + ((float)x / (float)nScreenWidth) * FOV;

			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundry = false;

			float fEyeX = sinf(fRayAngle);//Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);


			while (!bHitWall && fDistanceToWall<Depth) {
				fDistanceToWall += 0.1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				//Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					bHitWall = true; //just set distance to maximum depth
					fDistanceToWall = Depth;
				}else{
					//ray is inbound so test to see if the ray cell is a wall block
					if (map[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true;

						vector<pair<float, float>> p;//distance, dot

						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}

						//sort pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

						float fBound = 0.005;
						if (acos(p.at(0).second) < fBound)bBoundry = true;
						if (acos(p.at(1).second) < fBound)bBoundry = true;
						if (acos(p.at(2).second) < fBound)bBoundry = true;
					}

				}
			}

			//calculate distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';
			short nShadeFloor = ' ';
			short nShadeCeiling = ' ';


			if (fDistanceToWall <= Depth / 4.0f)		nShade = 0x2588; //very close
			else if (fDistanceToWall < Depth / 3.0f)	nShade = 0x2593;
			else if (fDistanceToWall < Depth / 2.0f)	nShade = 0x2592;
			else if (fDistanceToWall < Depth)			nShade = 0x2591;
			else										nShade = ' '; //too far away

			if (bBoundry) nShade = ' ';//black it out

			for (int y = 0; y < nScreenHeight; y++) {
				if (y < nCeiling) {
					screen[y * nScreenWidth + x] = nShadeCeiling;
				}
				else if (y > nCeiling && y <= nFloor) {
					screen[y * nScreenWidth + x] = nShade;
				}
				else {
					//Shade floor based on distance
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)		nShadeFloor = '#'; //very close
					else if (b < 0.5)	nShadeFloor = 'X';
					else if (b < 0.75)	nShadeFloor = '.';
					else if (b < 0.9)	nShadeFloor = '-';
					else				nShadeFloor = ' '; //too far away
					screen[y * nScreenWidth + x] = nShadeFloor;
				}
			}
		}
		/*
		//display stats
		swprintf_s(screen, 40, L"X=¡%3.2f, Y=¡%3.2f A=¡%3.2f FPS=¡%3.2f", fPlayerX, fPlayerY, fPlayerLook, 1.0f /fElapsedTime);

		//Display map
		for (int nx = 0; nx < nMapWidth; nx++) {
			for (int ny = 0; ny < nMapHeight; ny++) {
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		}
		
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';
		*/

		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);

	}
	
}
