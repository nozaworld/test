#include <curses.h>
#include <stdlib.h>
#include <ctime>

// ぷよの色を表すの列挙型
// NONEが無し，RED,BLUE,..が色を表す
enum puyocolor { NONE, RED, BLUE, GREEN, YELLOW };

class PuyoArray {
    private:
        puyocolor *data;
        unsigned int data_line;
        unsigned int data_column;

        void Release() {
            if (data == NULL) {
                return;
            }

            delete[] data;
            data = NULL;
        }

    public:
        void ChangeSize(unsigned int line, unsigned int column) {
            Release();
            data = new puyocolor[line * column];
            data_line = line;
            data_column = column;
        }

        unsigned int GetLine() { return data_line; }
        unsigned int GetColumn() { return data_column; }

        puyocolor GetValue(unsigned int y, unsigned int x) {
            if (y >= GetLine() || x >= GetColumn()) {
                return NONE;
            }
            return data[y * GetColumn() + x];
        }

        void SetValue(unsigned int y, unsigned int x, puyocolor value) {
            if (y < GetLine() && x < GetColumn()) {
                data[y * GetColumn() + x] = value;
            }
        }

        PuyoArray() {
            data = NULL;
            data_line = 0;
            data_column = 0;
        }

        ~PuyoArray() { Release(); }
};

class PuyoArrayActive : public PuyoArray {
    public:

        int puyorotate;

        PuyoArrayActive() : puyorotate(0) {}

};

class PuyoArrayStack : public PuyoArray {
    public:
        static const int LANDING_PUYO_RED = 1;
        static const int LANDING_PUYO_BLUE = 2;
        static const int LANDING_PUYO_GREEN = 3;
        static const int LANDING_PUYO_YELLOW = 4;

        void DisplayStack() {
            for (int y = 0; y < GetLine(); y++) {
                for (int x = 0; x < GetColumn(); x++) {
                    puyocolor color = GetValue(y, x);
                    if (color != NONE) {
                        switch (color) {
                            case RED:
                                init_pair(LANDING_PUYO_RED, COLOR_RED, COLOR_BLACK);
                                attrset(COLOR_PAIR(LANDING_PUYO_RED));
                                mvaddch(y, x, 'R');
                                break;
                            case BLUE:
                                init_pair(LANDING_PUYO_BLUE, COLOR_BLUE, COLOR_BLACK);
                                attrset(COLOR_PAIR(LANDING_PUYO_BLUE));
                                mvaddch(y, x, 'B');
                                break;
                            case GREEN:
                                init_pair(LANDING_PUYO_GREEN, COLOR_GREEN, COLOR_BLACK);
                                attrset(COLOR_PAIR(LANDING_PUYO_GREEN));
                                mvaddch(y, x, 'G');
                                break;
                            case YELLOW:
                                init_pair(LANDING_PUYO_YELLOW, COLOR_YELLOW, COLOR_BLACK);
                                attrset(COLOR_PAIR(LANDING_PUYO_YELLOW));
                                mvaddch(y, x, 'Y');
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
};

class PuyoControl {
    public:
        PuyoArrayActive &puyo;

        PuyoControl(PuyoArrayActive &puyo) : puyo(puyo) {}

        void GeneratePuyo(PuyoArrayActive &puyo) {
            puyocolor newpuyo1;
            puyo.puyorotate = 0;
            int ct = static_cast<double>(rand()) / RAND_MAX * 4.0;
            switch (ct) {
                case 0:
                    newpuyo1 = RED;
                    break;
                case 1:
                    newpuyo1 = BLUE;
                    break;
                case 2:
                    newpuyo1 = GREEN;
                    break;
                case 3:
                    newpuyo1 = YELLOW;
                    break;
            }

            puyocolor newpuyo2;
            int ct2 = static_cast<double>(rand()) / RAND_MAX * 4.0;
            switch (ct2) {
                case 0:
                    newpuyo2 = RED;
                    break;
                case 1:
                    newpuyo2 = BLUE;
                    break;
                case 2:
                    newpuyo2 = GREEN;
                    break;
                case 3:
                    newpuyo2 = YELLOW;
                    break;
            }

            puyo.SetValue(0, 6, newpuyo1);
            puyo.SetValue(0, 7, newpuyo2);
        }

        bool LandingPuyo(PuyoArrayActive &activePuyo, PuyoArrayStack &stackPuyo) {
            bool landed = false;

            for (int y = activePuyo.GetLine() - 1; y >= 0; y--) {//この行と下の行でフィールド内の全探索
                for (int x = 0; x < activePuyo.GetColumn(); x++) {
                    if (activePuyo.GetValue(y, x) != NONE) {//ここでアクティブなぷよを見つける
                        if (y == activePuyo.GetLine() - 1 || stackPuyo.GetValue(y + 1, x) != NONE) {//そのぷよが最下層にいるorその下がスタックぷよのとき
                                stackPuyo.SetValue(y, x, activePuyo.GetValue(y, x));//そのアクティブぷよの位置にスタックぷよを置く
                                activePuyo.SetValue(y, x, NONE);//上の処理だけだとアクティブぷよが残ってしまうから、アクティブぷよを消す
                                landed = true;
                        }
                    }
                }
            }

            return landed;
        }
        bool UnLandingPuyo(PuyoArrayActive &activePuyo, PuyoArrayStack &stackPuyo) {//この関数はUnLandingPuyoの逆でアクティブとスタックを入れ替えるだけ
            bool landed = false;

            for (int y = stackPuyo.GetLine() - 1; y >= 0; y--) {
                for (int x = 0; x < stackPuyo.GetColumn(); x++) {
                    if (stackPuyo.GetValue(y, x) != NONE) {
                        // ぷよが最下部に到達、または下に他のぷよがある場合
                        if (y != stackPuyo.GetLine() - 1 && stackPuyo.GetValue(y + 1, x) == NONE) {
                            activePuyo.SetValue(y, x, stackPuyo.GetValue(y, x));
                            stackPuyo.SetValue(y, x, NONE);
                            landed = true;
                        } 
                    }
                }
            }

            return landed;
        }   


        void MoveLeft(PuyoArrayActive &puyo,PuyoArrayStack &puyo2) {
            puyocolor *puyo_temp = new puyocolor[puyo.GetLine() * puyo.GetColumn()];

            for (int i = 0; i < puyo.GetLine() * puyo.GetColumn(); i++) {
                puyo_temp[i] = NONE;
            }

            for (int y = 0; y < puyo.GetLine(); y++) {
                for (int x = 0; x < puyo.GetColumn(); x++) {
                    if (puyo.GetValue(y, x) == NONE) {
                        continue;
                    }

                    if (0 < x && puyo.GetValue(y, x - 1) == NONE&& puyo2.GetValue(y, x - 1) == NONE) {
                        puyo_temp[y * puyo.GetColumn() + (x - 1)] = puyo.GetValue(y, x);
                        puyo.SetValue(y, x, NONE);
                    } else {
                        puyo_temp[y * puyo.GetColumn() + x] = puyo.GetValue(y, x);
                    }
                }
            }

            for (int y = 0; y < puyo.GetLine(); y++) {
                for (int x = 0; x < puyo.GetColumn(); x++) {
                    puyo.SetValue(y, x, puyo_temp[y * puyo.GetColumn() + x]);
                }
            }

            delete[] puyo_temp;
        }

        void MoveRight(PuyoArrayActive &puyo,PuyoArrayStack &puyo2) {
            puyocolor *puyo_temp = new puyocolor[puyo.GetLine() * puyo.GetColumn()];

            for (int i = 0; i < puyo.GetLine() * puyo.GetColumn(); i++) {
                puyo_temp[i] = NONE;
            }

            for (int y = 0; y < puyo.GetLine(); y++) {
                for (int x = puyo.GetColumn() - 1; x >= 0; x--) {
                    if (puyo.GetValue(y, x) == NONE) {
                        continue;
                    }

                    if (x < puyo.GetColumn() - 1 && puyo.GetValue(y, x + 1) == NONE&& puyo2.GetValue(y, x + 1) == NONE) {
                        puyo_temp[y * puyo.GetColumn() + (x + 1)] = puyo.GetValue(y, x);
                        puyo.SetValue(y, x, NONE);
                    } else {
                        puyo_temp[y * puyo.GetColumn() + x] = puyo.GetValue(y, x);
                    }
                }
            }

            for (int y = 0; y < puyo.GetLine(); y++) {
                for (int x = 0; x < puyo.GetColumn(); x++) {
                    puyo.SetValue(y, x, puyo_temp[y * puyo.GetColumn() + x]);
                }
            }

            delete[] puyo_temp;
        }

        void MoveDown(PuyoArrayActive &puyo) {
            puyocolor *puyo_temp = new puyocolor[puyo.GetLine() * puyo.GetColumn()];

            for (int i = 0; i < puyo.GetLine() * puyo.GetColumn(); i++) {
                puyo_temp[i] = NONE;
            }

            for (int y = puyo.GetLine() - 1; y >= 0; y--) {
                for (int x = 0; x < puyo.GetColumn(); x++) {
                    if (puyo.GetValue(y, x) == NONE) {
                        continue;
                    }

                    if (y < puyo.GetLine() - 1 && puyo.GetValue(y + 1, x) == NONE) {
                        puyo_temp[(y + 1) * puyo.GetColumn() + x] = puyo.GetValue(y, x);
                        puyo.SetValue(y, x, NONE);
                    } else {
                        puyo_temp[y * puyo.GetColumn() + x] = puyo.GetValue(y, x);
                    }
                }
            }

            for (int y = 0; y < puyo.GetLine(); y++) {
                for (int x = 0; x < puyo.GetColumn(); x++) {
                    puyo.SetValue(y, x, puyo_temp[y * puyo.GetColumn() + x]);
                }
            }

            delete[] puyo_temp;
        }
        


	//ぷよ消滅処理を全座標で行う
	//消滅したぷよの数を返す
	int VanishPuyo(PuyoArrayStack &puyostack)
	{
		int vanishednumber = 0;
		for (int y = 0; y < puyostack.GetLine(); y++)
		{
			for (int x = 0; x < puyostack.GetColumn(); x++)
			{
				vanishednumber += VanishPuyo(puyostack, y, x);
			}
		}

		return vanishednumber*vanishednumber*vanishednumber;
	}

	//ぷよ消滅処理を座標(x,y)で行う
	//消滅したぷよの数を返す
	int VanishPuyo(PuyoArrayStack &puyostack, unsigned int y, unsigned int x)
	{
		//判定個所にぷよがなければ処理終了
		if (puyostack.GetValue(y, x) == NONE)
		{
			return 0;
		}


		//判定状態を表す列挙型
		//NOCHECK判定未実施，CHECKINGが判定対象，CHECKEDが判定済み
		enum checkstate{ NOCHECK, CHECKING, CHECKED };

		//判定結果格納用の配列
		enum checkstate *field_array_check;
		field_array_check = new enum checkstate[puyostack.GetLine()*puyostack.GetColumn()];

		//配列初期化
		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			field_array_check[i] = NOCHECK;
		}

		//座標(x,y)を判定対象にする
		field_array_check[y*puyostack.GetColumn() + x] = CHECKING;

		//判定対象が1つもなくなるまで，判定対象の上下左右に同じ色のぷよがあるか確認し，あれば新たな判定対象にする
		bool checkagain = true;
		while (checkagain)
		{
			checkagain = false;

			for (int yy = 0; yy < puyostack.GetLine(); yy++)
			{
				for (int xx = 0; xx < puyostack.GetColumn(); xx++)
				{
					//(xx,yy)に判定対象がある場合
					if (field_array_check[yy*puyostack.GetColumn() + xx] == CHECKING)
					{
						//(xx+1,yy)の判定
						if (xx < puyostack.GetColumn() - 1)
						{
							//(xx+1,yy)と(xx,yy)のぷよの色が同じで，(xx+1,yy)のぷよが判定未実施か確認
							if (puyostack.GetValue(yy, xx + 1) == puyostack.GetValue(yy, xx) && field_array_check[yy*puyostack.GetColumn() + (xx + 1)] == NOCHECK)
							{
								//(xx+1,yy)を判定対象にする
								field_array_check[yy*puyostack.GetColumn() + (xx + 1)] = CHECKING;
								checkagain = true;
							}
						}

						//(xx-1,yy)の判定
						if (xx > 0)
						{
							if (puyostack.GetValue(yy, xx - 1) == puyostack.GetValue(yy, xx) && field_array_check[yy*puyostack.GetColumn() + (xx - 1)] == NOCHECK)
							{
								field_array_check[yy*puyostack.GetColumn() + (xx - 1)] = CHECKING;
								checkagain = true;
							}
						}

						//(xx,yy+1)の判定
						if (yy < puyostack.GetLine() - 1)
						{
							if (puyostack.GetValue(yy + 1, xx) == puyostack.GetValue(yy, xx) && field_array_check[(yy + 1)*puyostack.GetColumn() + xx] == NOCHECK)
							{
								field_array_check[(yy + 1)*puyostack.GetColumn() + xx] = CHECKING;
								checkagain = true;
							}
						}

						//(xx,yy-1)の判定
						if (yy > 0)
						{
							if (puyostack.GetValue(yy - 1, xx) == puyostack.GetValue(yy, xx) && field_array_check[(yy - 1)*puyostack.GetColumn() + xx] == NOCHECK)
							{
								field_array_check[(yy - 1)*puyostack.GetColumn() + xx] = CHECKING;
								checkagain = true;
							}
						}

						//(xx,yy)を判定済みにする
						field_array_check[yy*puyostack.GetColumn() + xx] = CHECKED;
					}
				}
			}
		}

		//判定済みの数をカウント
		int puyocount = 0;
		for (int i = 0; i < puyostack.GetLine()*puyostack.GetColumn(); i++)
		{
			if (field_array_check[i] == CHECKED)
			{
				puyocount++;
			}
		}

		//4個以上あれば，判定済み座標のぷよを消す
		int vanishednumber = 0;
		if (4 <= puyocount)
		{
			for (int yy = 0; yy < puyostack.GetLine(); yy++)
			{
				for (int xx = 0; xx < puyostack.GetColumn(); xx++)
				{
					if (field_array_check[yy*puyostack.GetColumn() + xx] == CHECKED)
					{
						puyostack.SetValue(yy, xx, NONE);

						vanishednumber++;
					}
				}
			}
		}

		//メモリ解放
		delete[] field_array_check;

		return vanishednumber;
	}

    	//回転
	//PuyoArrayActiveクラスのprivateメンバ変数として int puyorotate を宣言し，これに回転状態を記憶させている．
	//puyorotateにはコンストラクタ及びGeneratePuyo関数で値0を代入する必要あり．
	void Rotate(PuyoArrayActive &puyoactive,PuyoArrayStack &puyostack)
	{
		//フィールドをラスタ順に探索（最も上の行を左から右方向へチェックして，次に一つ下の行を左から右方向へチェックして，次にその下の行・・と繰り返す）し，先に発見される方をpuyo1, 次に発見される方をpuyo2に格納
		puyocolor puyo1, puyo2;
		int puyo1_x = 0;
		int puyo1_y = 0;
		int puyo2_x = 0;
		int puyo2_y = 0;

		bool findingpuyo1 = true;
		for (int y = 0; y < puyoactive.GetLine(); y++)
		{
			for (int x = 0; x < puyoactive.GetColumn(); x++)
			{
				if (puyoactive.GetValue(y, x) != NONE)
				{
					if (findingpuyo1)
					{
						puyo1 = puyoactive.GetValue(y, x);
						puyo1_x = x;
						puyo1_y = y;
						findingpuyo1 = false;
					}
					else
					{
						puyo2 = puyoactive.GetValue(y, x);
						puyo2_x = x;
						puyo2_y = y;
					}
				}
			}
		}


		//回転前のぷよを消す
		puyoactive.SetValue(puyo1_y, puyo1_x, NONE);
		puyoactive.SetValue(puyo2_y, puyo2_x, NONE);


		//操作中ぷよの回転
		switch (puyoactive.puyorotate)
		{
		case 0:
			//回転パターン
			//RB -> R
			//      B
			//Rがpuyo1, Bがpuyo2
			if (puyo2_x <= 0 || puyo2_y >= puyoactive.GetLine() - 1||puyostack.GetValue(puyo1_y+1, puyo1_x) != NONE)	//もし回転した結果field_arrayの範囲外に出るなら回転しない
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				break;
			}

			//回転後の位置にぷよを置く
			puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
			puyoactive.SetValue(puyo2_y + 1, puyo2_x - 1, puyo2);
			//次の回転パターンの設定
			puyoactive.puyorotate = 1;
			break;

		case 1:
			//回転パターン
			//R -> BR
			//B
			//Rがpuyo1, Bがpuyo2
			if (puyo2_x <= 0 || puyo2_y <= 0||puyostack.GetValue(puyo1_y, puyo1_x-1) != NONE)	//もし回転した結果field_arrayの範囲外に出るなら回転しない
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				break;
			}

			//回転後の位置にぷよを置く
			puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
			puyoactive.SetValue(puyo2_y - 1, puyo2_x - 1, puyo2);

			//次の回転パターンの設定
			puyoactive.puyorotate = 2;
			break;

		case 2:
			//回転パターン
			//      B
			//BR -> R
			//Bがpuyo1, Rがpuyo2
			if (puyo1_x >= puyoactive.GetColumn() - 1 || puyo1_y <= 0||puyostack.GetValue(puyo2_y-1, puyo2_x) != NONE)	//もし回転した結果field_arrayの範囲外に出るなら回転しない
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				break;
			}

			//回転後の位置にぷよを置く
			puyoactive.SetValue(puyo1_y - 1, puyo1_x + 1, puyo1);
			puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);

			//次の回転パターンの設定
			puyoactive.puyorotate = 3;
			break;

		case 3:
			//回転パターン
			//B
			//R -> RB
			//Bがpuyo1, Rがpuyo2
			if (puyo1_x >= puyoactive.GetColumn() - 1 || puyo1_y >= puyoactive.GetLine() - 1||puyostack.GetValue(puyo2_y, puyo2_x+1) != NONE)	//もし回転した結果field_arrayの範囲外に出るなら回転しない
			{
				puyoactive.SetValue(puyo1_y, puyo1_x, puyo1);
				puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);
				break;
			}

			//回転後の位置にぷよを置く
			puyoactive.SetValue(puyo1_y + 1, puyo1_x + 1, puyo1);
			puyoactive.SetValue(puyo2_y, puyo2_x, puyo2);

			//次の回転パターンの設定
			puyoactive.puyorotate = 0;
			break;

		default:
			break;
		}
	}


};

void Display(PuyoArrayActive &activePuyo, PuyoArrayStack &stackPuyo,int score) {
    // 落下中のぷよを表示
    for (int y = 0; y < activePuyo.GetLine(); y++) {
        for (int x = 0; x < activePuyo.GetColumn(); x++) {
            switch (activePuyo.GetValue(y, x)) {
                case NONE:
                    init_pair(0, COLOR_WHITE, COLOR_BLACK);
                    attrset(COLOR_PAIR(0));
                    mvaddch(y, x, '.');
                    break;
                case RED:
                    init_pair(1, COLOR_RED, COLOR_BLACK);
                    attrset(COLOR_PAIR(1));
                    mvaddch(y, x, 'R');
                    break;
                case BLUE:
                    init_pair(2, COLOR_BLUE, COLOR_BLACK);
                    attrset(COLOR_PAIR(2));
                    mvaddch(y, x, 'B');
                    break;
                case GREEN:
                    init_pair(3, COLOR_GREEN, COLOR_BLACK);
                    attrset(COLOR_PAIR(3));
                    mvaddch(y, x, 'G');
                    break;
                case YELLOW:
                    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
                    attrset(COLOR_PAIR(4));
                    mvaddch(y, x, 'Y');
                    break;
            }
        }
    }

    // 着地済みのぷよを表示
    stackPuyo.DisplayStack();

    int count = 0;
    for (int y = 0; y < activePuyo.GetLine(); y++) {
        for (int x = 0; x < activePuyo.GetColumn(); x++) {
            if (activePuyo.GetValue(y, x) != NONE) {
                count++;
            }
            if (stackPuyo.GetValue(y, x) != NONE) {
                count++;
            }
        }
    }

    char msg[256];
    
    attrset(COLOR_PAIR(0));
    sprintf(msg, "Field: %d x %d, Puyo number: %03d, Your score is %06d.", activePuyo.GetLine(), activePuyo.GetColumn(), count,score);
    
    mvaddstr(2, COLS - 65, msg);

    refresh();
}

int main(int argc, char **argv) {
    long int score=0;
    PuyoArrayActive activePuyo;
    PuyoArrayStack stackPuyo;
    PuyoControl control(activePuyo);

    srand(time(NULL));
    initscr();
    start_color();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(0);

    activePuyo.ChangeSize(LINES / 2, COLS / 2);
    stackPuyo.ChangeSize(LINES / 2, COLS / 2); // スタック用のサイズも変更

    control.GeneratePuyo(activePuyo);

    int delay = 0;
    int kskst=0;
    int waitCount = 12000-kskst;
    int smallscore=0;

    while (1) {
        int ch = getch();
        

        if (ch == 'Q') {
            break;
        }
        

        switch (ch) {
            case KEY_LEFT:
                control.MoveLeft(activePuyo,stackPuyo);
                break;
            case KEY_RIGHT:
                control.MoveRight(activePuyo,stackPuyo);
                break;
            case 'z':
                break;
            case KEY_UP:
                control.Rotate(activePuyo,stackPuyo);
                break;
            case KEY_DOWN:
                delay=0;
                break;
            
            
            default:
                break;
        }

        if (delay % waitCount == 0) {
            control.MoveDown(activePuyo);
            
            if (control.LandingPuyo(activePuyo, stackPuyo)) {
                // activePuyoの個数をカウント
                smallscore*=2;
                smallscore+=control.VanishPuyo(stackPuyo);
                
                control.UnLandingPuyo(activePuyo, stackPuyo);
                int activePuyoCount = 0;
                for (int y = 0; y < activePuyo.GetLine(); y++) {
                    for (int x = 0; x < activePuyo.GetColumn(); x++) {
                        if (activePuyo.GetValue(y, x) != NONE) {
                            activePuyoCount++;
                        }
                    }
                }
                

                // activePuyoの個数が0の場合にtrue
                if (activePuyoCount == 0) {
                    control.GeneratePuyo(activePuyo);
                    if(kskst>1000){
                        kskst+=500;
                    }
                    score+=smallscore;
                    smallscore=0;
                }
                
            }
        }

        delay++;

        Display(activePuyo, stackPuyo,score);
        
    }

    endwin();

    return 0;
}