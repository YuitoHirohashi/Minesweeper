#include "DxLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define VSIZE 15
#define HSIZE 25
#define BOM (VSIZE)*(HSIZE)*(0.15)
#define FLAG (VSIZE)*(HSIZE)*(0.15)

int retry_flag = 0; //リトライフラグ
int first_click = 0; // 初めのクリック
double start;
double record = 0;

int bgm; // BGM
int clear_se; // クリア時に流れるSE
int se_flag = 0; // SEが鳴ったかどうかの判定

int Key[256]; // キーが押されているフレーム数を格納する

int gpUpdateKey(); // キーの入力状態を更新する
void Detection(int mine[][HSIZE + 2], int x, int y); //周囲の爆弾の検知
void Usa(int& usax, int& usay, int usa);
void Dig(int x, int y, int mine[][HSIZE + 2], int object[][HSIZE + 2]);
void Click(int object[][HSIZE + 2], int mine[][HSIZE + 2], int usax, int usay, int* flag_num); //クリック
void Display(int f[][HSIZE + 2], int mine[][HSIZE + 2], int object[][HSIZE + 2], int F[], int cal, int ground, int flag, int flag_num, double time); //マップの表示
void Judge(int mine[][HSIZE + 2], int object[][HSIZE + 2], int clear, int* judge); //終了判定する関数
void Retry(int* judge, int f[][HSIZE + 2], int mine[][HSIZE + 2], int object[][HSIZE + 2], int* flag_num, int* clear, double time);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE), DxLib_Init(), SetDrawScreen(DX_SCREEN_BACK); //ウィンドウモード変更と初期化と裏画面設定

    srand((unsigned int)time(NULL));

    int f[VSIZE + 2][HSIZE + 2], //フィールド
        mine[VSIZE + 2][HSIZE + 2], //地面を堀ったときの表示(その周りに爆弾が何個あるかの表示)
        object[VSIZE + 2][HSIZE + 2], //爆弾やフラグ、掘られてない地面などの情報
        pm, //新たに爆弾を設置する前の地面の値

        /*座標系*/
        usax, usay, //ウサギの座標
        bomx, bomy, //爆弾を設置する座標

        /*画像系*/
        F[3]; //マップ画像を格納
    
    /*BGM*/
    bgm = LoadSoundMem("famipop3.mp3"); //BGMの読み込み
    clear_se = LoadSoundMem("clear.mp3"); //SEの読み込み
    ChangeVolumeSoundMem(60, bgm); //BGMの音量の調製
    ChangeVolumeSoundMem(80, clear_se); //BGMの音量の調製


    int i, j;
    int bom = 0, //設置した爆弾の個数を表す変数
        flag_num, //立てたフラグの数(爆弾の総数)
        clear, //クリア判定
        judge = 0; //終了判定

    double time = 0; // 時間

    LoadDivGraph("map.png", 3, 3, 1, 32, 32, F); //マップ画像の読み込み

    int cal = LoadGraph("cal.png"), //爆弾人参の画像の読み込み
        ground = LoadGraph("ground.png"), //地面画像の読み込み
        flag = LoadGraph("flag.png"), //フラグ画像の読み込み
        usa = LoadGraph("usa.png"); //うさぎの画像を格納

    for (i = 0; i < VSIZE + 2; i++) { //外枠と内側(掘られてない地面)の初期化
        for (j = 0; j < HSIZE + 2; j++) {
            f[i][j] = 2; //壁の値 = 2
            mine[i][j] = 0; //爆弾が設置されてない地面の初期化
            object[i][j] = 0; //掘られてない地面の初期化
            if ((1 <= i && i <= VSIZE) && (1 <= j && j <= HSIZE)) {
                f[i][j] = 0; //掘られた地面の初期化 = 0
            }

        }
    }

    while (bom < BOM) { //爆弾をすべて設置し終わるまで処理を繰り返す
        for (i = 1; i <= VSIZE; i++) { //内側の爆弾の設置
            for (j = 1; j < HSIZE; j++) {
                bomx = rand() % VSIZE + 1;
                bomy = rand() % HSIZE + 1;
                pm = mine[bomx][bomy]; //爆弾を設置する前の配列の要素の値
                if (pm != -1 && bom < BOM) { //爆弾が設置されてないときの処理
                    mine[bomx][bomy] = -1; //爆弾の設置
                    bom++;
                }
            }
        }
    }

    flag_num = bom; //設置した爆弾の数だけフラグを用意する
    clear = (VSIZE) * (HSIZE)-bom; //爆弾じゃないとこの数   

    for (i = 1; i <= VSIZE; i++) {
        for (j = 1; j <= HSIZE; j++) {
            if (mine[i][j] == 0) Detection(mine, i, j); //周囲の爆弾の検知
        }
    }
    

    // while(裏画面を表画面に反映, メッセージ処理, 画面クリア)
    while (ScreenFlip() == 0 && ProcessMessage() == 0 && ClearDrawScreen() == 0 && gpUpdateKey() == 0) {
        Display(f, mine, object, F, cal, ground, flag, flag_num, time); //マップの表示  
        Usa(usax, usay, usa); //ウサギの移動
        if (judge == 0) {
            Click(object, mine, usax, usay, &flag_num); //クリック操作
            time = GetNowCount() - start;
        }
        Judge(mine, object, clear, &judge); //終了判定
        if (judge && Key[KEY_INPUT_SPACE]) { //終了時にスペースキーを押してリトライ
            Retry(&judge, f, mine, object, &flag_num, &clear, time);
            start = GetNowCount();
            time = 0;
        }

    }

    DxLib_End(); // DXライブラリ終了処理
    return 0;
}

// キーの入力状態を更新する
int gpUpdateKey() {
    char tmpKey[256]; // 現在のキーの入力状態を格納する
    GetHitKeyStateAll(tmpKey); // 全てのキーの入力状態を得る
    for (int i = 0; i < 256; i++) {
        if (tmpKey[i] != 0) { // i番のキーコードに対応するキーが押されていたら
            Key[i]++;     // 加算
        }
        else {              // 押されていなければ
            Key[i] = 0;   // 0にする
        }
    }
    return 0;
}


//周囲の爆弾を検知する関数
void Detection(int mine[][HSIZE + 2], int x, int y) { //周囲の爆弾の検知
    int i, j,
        count = 0;

    for (i = x - 1; i <= x + 1; i++) {
        for (j = y - 1; j <= y + 1; j++) {
            if (mine[i][j] == -1) count++;
        }
    }

    mine[x][y] = count; //周囲の爆弾の個数
}


void Usa(int& usax, int& usay, int usa) {
    GetMousePoint(&usax, &usay);
    if (usay <= 34 && usax <= 29) DrawRotaGraph(29, 34, 0.75, 0.0, usa, TRUE);
    else if (usay >= 24 * VSIZE + 10 && usax <= 29) DrawRotaGraph(29, 24 * VSIZE + 10, 0.75, 0.0, usa, TRUE);
    else if (usax >= 24 * HSIZE + 10 && usay <= 34) DrawRotaGraph(24 * HSIZE + 10, 34, 0.75, 0.0, usa, TRUE);
    else if (usax >= 24 * HSIZE + 10 && usay >= 24 * VSIZE + 10) DrawRotaGraph(24 * HSIZE + 10, 24 * VSIZE + 10, 0.75, 0.0, usa, TRUE);
    else if (usay <= 34) DrawRotaGraph(usax, 34, 0.75, 0.0, usa, TRUE);
    else if (usax <= 29) DrawRotaGraph(29, usay, 0.75, 0.0, usa, TRUE);
    else if (usay >= 24 * VSIZE + 10) DrawRotaGraph(usax, 24 * VSIZE + 10, 0.75, 0.0, usa, TRUE);
    else if (usax >= 24 * HSIZE + 10) DrawRotaGraph(24 * HSIZE + 10, usay, 0.75, 0.0, usa, TRUE);
    else DrawRotaGraph(usax, usay, 0.75, 0.0, usa, TRUE);
}

//爆弾が設置されてない かつ　周囲に爆弾がないときに周囲の地面をまとめて掘る
void Dig(int x, int y, int mine[][HSIZE + 2], int object[][HSIZE + 2]) {

    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if ((i >= 1 && i <= VSIZE) && (j >= 1 && j <= HSIZE) && mine[i][j] != -1 && object[i][j] != 2) {
                object[i][j] = 1;
            }
        }
    }
}

//クリックに関する関数
void Click(int object[][HSIZE + 2], int mine[][HSIZE + 2], int usax, int usay, int* flag_num) {
    int Mouse = GetMouseInput();
    int i, j;

    if (Mouse & MOUSE_INPUT_LEFT) {
        if ((usax >= 24 && usax <= 24 * HSIZE + 15) && (usay >= 29 && usay <= 24 * VSIZE + 15)) {
            if (object[usay / 24][usax / 24] != 2) object[usay / 24][usax / 24] = 1; //フラグが立っていなければ地面を掘る
            if (mine[usay / 24][usax / 24] == 0) Dig(usay / 24, usax / 24, mine, object);
        }
        if (first_click == 0) {
            start = GetNowCount(); // 現在時刻の取得
            first_click++;
            PlaySoundMem(bgm, DX_PLAYTYPE_LOOP); //BGMの再生
        }
    }

    if (Mouse & MOUSE_INPUT_RIGHT) {
        switch (object[usay / 24][usax / 24]) { //後で調整(?)
        case 0:
            if ((*flag_num) > 0) {
                object[usay / 24][usax / 24] = 2;
                (*flag_num)--;
            }

            break;

        case 2:
            object[usay / 24][usax / 24] = 0;
            (*flag_num)++;
            break;
        }
    }

}


//画面を表示する関数
void Display(int f[][HSIZE + 2], int mine[][HSIZE + 2], int object[][HSIZE + 2], int F[], int cal, int ground, int flag, int flag_num, double time) {
    int i, j;

    for (i = 0; i < VSIZE + 2; i++) {
        for (j = 0; j < HSIZE + 2; j++) { //壁と肌色の部分の表示
            DrawRotaGraph(24 * j + 5, 24 * i + 10, 0.75, 0.0, F[f[i][j]], FALSE);
            //爆弾人参の表示
            if ((1 <= i && i <= VSIZE) && (1 <= j && j <= HSIZE) && mine[i][j] == -1) DrawRotaGraph(24 * j + 5, 24 * i + 10, 0.75, 0.0, cal, FALSE);
        }
    }

    for (i = 0; i < VSIZE + 2; i++) {
        for (j = 0; j < HSIZE + 2; j++) {
            if ((1 <= i && i <= VSIZE) && (1 <= j && j <= HSIZE) && mine[i][j] != 0 && mine[i][j] != -1) {
                DrawFormatString(24 * j, 24 * i + 5, GetColor(255, 0, 0), "%d", mine[i][j]);
            }
        }
    }

    //右クリックでフラグを立てる(別の場所で処理を記述) -> 配列の値を変化させる(ex : 2) -> 配列(object[][])の値が2ならフラグ(flag)を表示
    //既にフラグが立っている状態でもう一ど右クリック -> 配列の値を 2 -> 0 にする
    //左クリックで地面を掘る(右クリックと同じように処理を記述) -> 配列の値を変化させる(ex : 1) -> 配列(object[][])の値が1ならgroundを表示しない 
    for (i = 0; i < VSIZE + 2; i++) {
        for (j = 0; j < HSIZE + 2; j++) {
            if ((1 <= i && i <= VSIZE) && (1 <= j && j <= HSIZE) && object[i][j] == 0) DrawRotaGraph(24 * j + 5, 24 * i + 10, 0.75, 0.0, ground, FALSE);
            if ((1 <= i && i <= VSIZE) && (1 <= j && j <= HSIZE) && object[i][j] == 2) DrawRotaGraph(24 * j + 5, 24 * i + 10, 0.75, 0.0, flag, FALSE);
        }
    }
    DrawFormatString(50, 430, GetColor(0, 255, 0), "FLAG : %d", flag_num);
    if (first_click == 0)
        DrawFormatString(450, 430, GetColor(0, 255, 0), "TIME   :   %.2f", 0);
    else
        DrawFormatString(450, 430, GetColor(0, 255, 0), "TIME   :   %.2f", time * 0.001);
    DrawFormatString(450, 450, GetColor(0, 255, 0), "RECORD :   %.2f", record * 0.001);
}

//終了判定
void Judge(int mine[][HSIZE + 2], int object[][HSIZE + 2], int clear, int* judge) {
    int fin_1 = 0,
        fin_2 = 0;
    for (int i = 1; i <= VSIZE; i++) {
        for (int j = 1; j <= HSIZE; j++) {
            if (mine[i][j] == -1 && object[i][j] == 1) fin_1++;
            if (mine[i][j] != -1 && object[i][j] == 1) fin_2++;
        }
    }

    if (fin_2 == clear) {
        DrawFormatString(250, 430, GetColor(0, 255, 0), "CLEAR!");
        if (!se_flag) {
            PlaySoundMem(clear_se, DX_PLAYTYPE_BACK); //SEの再生 
            se_flag++;
        }
        StopSoundMem(bgm);
        *judge = 1;
    }
    if (fin_1 > 0) {
        DrawFormatString(250, 430, GetColor(255, 0, 0), "GAMEOVER!");
        DrawFormatString(250, 450, GetColor(255, 255, 0), "Push Space Key.");
        StopSoundMem(bgm);
        *judge = 2;
    }

}

void Retry(int* judge, int f[][HSIZE + 2], int mine[][HSIZE + 2], int object[][HSIZE + 2], int* flag_num, int* clear, double time) {

    for (int i = 0; i < VSIZE + 2; i++) { //外枠と内側(掘られてない地面)の初期化
        for (int j = 0; j < HSIZE + 2; j++) {
            f[i][j] = 2; //壁の値 = 2
            mine[i][j] = 0; //爆弾が設置されてない地面の初期化
            object[i][j] = 0; //掘られてない地面の初期化
            if ((1 <= i && i <= VSIZE) && (1 <= j && j <= HSIZE)) {
                f[i][j] = 0; //掘られた地面の初期化 = 0
            }

        }
    }

    int bom = 0, bomx, bomy, pm;

    while (bom < BOM) { //爆弾をすべて設置し終わるまで処理を繰り返す
        for (int i = 1; i <= VSIZE; i++) { //内側の爆弾の設置
            for (int j = 1; j < HSIZE; j++) {
                bomx = rand() % VSIZE + 1;
                bomy = rand() % HSIZE + 1;
                pm = mine[bomx][bomy]; //爆弾を設置する前の配列の要素の値
                if (pm != -1 && bom < BOM) { //爆弾が設置されてないときの処理
                    mine[bomx][bomy] = -1; //爆弾の設置
                    bom++;
                }
            }
        }
    }

    *flag_num = bom; //設置した爆弾の数だけフラグを用意する
    *clear = (VSIZE) * (HSIZE)-bom; //爆弾じゃないとこの数   

    for (int i = 1; i <= VSIZE; i++) {
        for (int j = 1; j <= HSIZE; j++) {
            if (mine[i][j] == 0) Detection(mine, i, j); //周囲の爆弾の検知
        }
    }

    if (record == 0 && *judge == 1)
        record = time;
    else if (record > time && *judge == 1)
        record = time;

    first_click = 0;

    se_flag = 0;

    *judge = 0;

}