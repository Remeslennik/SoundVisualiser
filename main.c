#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <math.h>
#include "portaudio.h"


#define FPS 100
#define AXIS_ZERO_SOUND 75
#define AXIS_ZERO_SMA 225
#define AXIS_ZERO_WIN 375
#define AXIS_ZERO_FFT 590
#define BLACK 0,0,0
#define BLUE 0,0,255
#define GREEN 0,200,50
#define RED 255,0,0
#define BROUN 20,100, 50
#define ORANGE 255,130,0
#define L_BLUE 0,220,220
#define L_GRAY 230,230,230


#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (2048) //по размеру экрана
#define POW_2 (11)
#define NUM_CHANNELS (1)
#define WINDOWMA (20)

/* Select sample format. */
#define PA_SAMPLE_TYPE paFloat32
#define SAMPLE_SIZE (4)
#define SAMPLE_SILENCE (0.0f)
#define CLEAR(a) memset( (a), 0, FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE ) // обнуление массива
#define PRINTF_S_FORMAT "%.8f"

#define  NUMBER_IS_2_POW_K(x)   ((!((x)&((x)-1)))&&((x)>1))  // x is pow(2, k), k=1,2, ...
#define  FT_DIRECT        -1    // Direct transform.
#define  FT_INVERSE        1    // Inverse transform.

int unsigned NextTick = 0 , interval = 1 * 1000 / FPS ;
SDL_Surface* screen;

void FPS_Fn();
void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B);
void DrawAxis();
void DrawWave(float* sampleBlock);
void DrawAxisSMA();
void SimpleMA(float *buf_1, int buf_size, int window);
void DrawSMA(float* sampleBlock);
void WIN_FUN(float *Index, int n);// коэффициенты оконной функции
void DrawAxisWIN();
void DrawWIN(float* sampleBlock);
void DrawAxisFFT();
int FFT(float *Rdat, float *Idat, int N, int LogN, int Ft_Flag); // преобразование фурье
void DrawFFT(float* sampleBlock);
void VerPar(float x1,float y1,float x2,float y2,float x3,float y3); // поиск вершины параболы
float X0, Y0, Freq; // координаты параболы
void DrawText();


void Slock(SDL_Surface *screen);
void Sulock(SDL_Surface *screen);

TTF_Font* my_font;
SDL_Surface* sound_txt;
SDL_Rect sound_pos = {50,8,0,0};
SDL_Surface* sma_txt;
SDL_Rect sma_pos = {50,160,0,0};
SDL_Surface* win_txt;
SDL_Rect win_pos = {50,310,0,0};
SDL_Surface* fft_txt;
SDL_Rect fft_pos = {50,434,0,0};
SDL_Surface* freq_txt;
SDL_Rect freq_pos = {50,474,0,0};
SDL_Surface* power_txt;
SDL_Rect power_pos = {50,490,0,0};

int main ( int argc, char** argv )
{
    PaStreamParameters inputParameters;
    PaStream *stream = NULL;
    float *sampleBlock;

    int numBytes;
    numBytes = FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE ;
    sampleBlock = (float *) malloc( numBytes );
    CLEAR(sampleBlock);

    // инициализация портаудио
    Pa_Initialize();
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // открытие потока
    Pa_OpenStream(
        &stream,
        &inputParameters,
        0,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        NULL,
        NULL );
    // старт потока
    Pa_StartStream( stream );

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }
    if (TTF_Init() == -1)
    {
        printf("Unable to initialize SDL_ttf: %s \n", TTF_GetError());
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // load font
    my_font = TTF_OpenFont("DejaVuSans-Bold.ttf", 15);
    if(my_font == NULL)
    {
        printf("Unable to load font: %s \n", TTF_GetError());
        return false;
    }
    SDL_Color blue = { BLUE };
    sound_txt = TTF_RenderUTF8_Blended(my_font, "SOUND WAVE", blue);
    SDL_Color green = { GREEN };
    sma_txt = TTF_RenderUTF8_Blended(my_font, "SMA", green);
    SDL_Color red = { RED };
    win_txt = TTF_RenderUTF8_Blended(my_font, "WINDOWED", red);
    SDL_Color broun = { BROUN };
    fft_txt = TTF_RenderUTF8_Blended(my_font, "FFT", broun);

    // create a new window
    screen = SDL_SetVideoMode(1200, 600, 32,
                              SDL_HWSURFACE|SDL_DOUBLEBUF);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    float WinFun[FRAMES_PER_BUFFER]; // массив коэффициентов оконной функции
    WIN_FUN(WinFun,FRAMES_PER_BUFFER); // вычисляем коэффициенты оконой функции
    int x;
    bool done = false;
    // program main loop

    while (!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;

                // check for keypresses
            case SDL_KEYDOWN:
            {
                // exit if ESCAPE is pressed
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    done = true;
                break;
            }
            } // end switch
        } // end of message processing


        Pa_ReadStream( stream, sampleBlock, FRAMES_PER_BUFFER ); // захват звука


        // DRAWING STARTS HERE ///////////////////////////////////////

        // clear screen
        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 255, 255, 255));

        DrawAxis();  //рисуем оси для волны
        DrawWave(sampleBlock); // график волны звука
        DrawAxisSMA(); // оси для скользящей средней
        SimpleMA( sampleBlock, FRAMES_PER_BUFFER, WINDOWMA); // фильтр скользящее среднее
        DrawSMA(sampleBlock); // график волны фильтрованного
        for (x=0; x<FRAMES_PER_BUFFER; ++x) sampleBlock[x]*=WinFun[x];// применяем оконную функцию
        DrawAxisWIN(); // оси для оконной
        DrawWIN(sampleBlock); // с примененной оконной функцией
        DrawAxisFFT(); // оси для фурье
        DrawFFT(sampleBlock); // рисуем спектр
        DrawText(); // рисуем надписи и числа



        // DRAWING ENDS HERE

        // finally, update the screen :)
        SDL_Flip(screen);
        FPS_Fn();
    } // end main loop

    // free loaded surfaces
    SDL_FreeSurface(sound_txt);
    SDL_FreeSurface(sma_txt);
    SDL_FreeSurface(win_txt);
    SDL_FreeSurface(fft_txt);
    SDL_FreeSurface(freq_txt);
    SDL_FreeSurface(power_txt);
    TTF_CloseFont (my_font);
    TTF_Quit();

    // all is well ;)
    printf("Exited cleanly\n");
    return 0;
}

void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B)
{

    Uint32 color = SDL_MapRGB(screen->format, R, G, B);
    switch (screen->format->BytesPerPixel)
    {
    case 1:  // Assuming 8-bpp
    {
        Uint8 *bufp;
        bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
        *bufp = color;
    }
    break;
    case 2: // Probably 15-bpp or 16-bpp
    {
        Uint16 *bufp;
        bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
        *bufp = color;
    }
    break;
    case 3: // Slow 24-bpp mode, usually not used
    {
        Uint8 *bufp;
        bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
        if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
        {
            bufp[0] = color;
            bufp[1] = color >> 8;
            bufp[2] = color >> 16;
        }
        else
        {
            bufp[2] = color;
            bufp[1] = color >> 8;
            bufp[0] = color >> 16;
        }
    }
    break;
    case 4: // Probably 32-bpp
    {
        Uint32 *bufp;
        bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
        *bufp = color;
    }
    break;
    }

}

void FPS_Fn(void)
{
    // NextTick indicate the next tick to achive FPS.   After run a loop & back to here ,
    // if the program code in this loop is not heavy , the current tick should smaller than
    // NextTick.   If this happen , call SDL_Delay to release CPU resouces.
    if ( NextTick > SDL_GetTicks( ) ) SDL_Delay( NextTick - SDL_GetTicks( ) );
    NextTick = SDL_GetTicks( ) + interval ;
    return;
}

void DrawAxis()
{
    int x;
    //Slock(screen);
    for(x=0; x<1024; x++)
    {
        // ось нулевая звука
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SOUND   ,100,100,100);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SOUND-70,200,200,200);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SOUND+70,200,200,200);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SOUND-10,220,220,220);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SOUND+10,220,220,220);
    }
    //Sulock(screen);
}

void DrawAxisSMA()
{
    int x;
    //Slock(screen);
    for(x=0; x<1024; x++)
    {
        // ось нулевая звука
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SMA   ,100,100,100);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SMA-70,200,200,200);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SMA+70,200,200,200);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SMA-10,220,220,220);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_SMA+10,220,220,220);
    }
}

void DrawAxisWIN()
{
    int x;
    //Slock(screen);
    for(x=0; x<1024; x++)
    {
        // ось нулевая звука
        DrawPixel(screen, 50 + x ,AXIS_ZERO_WIN   ,100,100,100);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_WIN-70,200,200,200);
        //DrawPixel(screen, 50 + x ,AXIS_ZERO_WIN+70,200,200,200);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_WIN-10,220,220,220);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_WIN+10,220,220,220);
    }
}

void DrawAxisFFT()
{
    int x;
    //Slock(screen);
    for(x=0; x<1024; x++)
    {
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT   , BLACK);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-20, L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-40,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-60,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-80,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-100,L_BLUE);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-120,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-140,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-160,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-180,L_GRAY);
        DrawPixel(screen, 50 + x ,AXIS_ZERO_FFT-200,L_BLUE);


    }
}

void DrawWave(float* sampleBlock)
{
    int x=0, FirstZero=0;
    // первое ненулевое слева
    for(x=1; x<FRAMES_PER_BUFFER; x++)// со второго значения
    {
        if (sampleBlock[x]>=0 && sampleBlock[x-1]<0)
        {
            FirstZero=x;
            break;
        }
    }

    for( x=FirstZero; x<FRAMES_PER_BUFFER; x++)
    {
        DrawPixel(screen, 50 + (x-FirstZero)/2.  , AXIS_ZERO_SOUND - sampleBlock[x]*70 , BLUE);
    }

}

void SimpleMA(float *buf_1, int buf_size, int window)
{
    int i;
    float buf_2[buf_size];
    float sum=0;
    // начальные данные
    for (i=0; i<window; i++)
    {
        sum+=buf_1[i];
        buf_2[i]=sum/(i+1);
    }
    //основной буфер
    for (i=window; i<buf_size; i++)
    {
        sum-=buf_1[i-window]; // рекурентно, вычитаем старое значение
        sum+=buf_1[i]; // прибавляем новое
        buf_2[i]=sum/window;
    }

    for (i=0; i<buf_size; i++) buf_1[i]=buf_2[i];
    return;
}

void DrawSMA(float* sampleBlock)
{
    int x=0, FirstZero=0;
    // первое ненулевое слева
    for(x=1; x<FRAMES_PER_BUFFER; x++)// со второго значения
    {
        if (sampleBlock[x]>=0 && sampleBlock[x-1]<0)
        {
            FirstZero=x;
            break;
        }
    }

    for( x=FirstZero; x<FRAMES_PER_BUFFER; x++)
    {
        DrawPixel(screen, 50 + (x-FirstZero)/2.  , AXIS_ZERO_SMA - sampleBlock[x]*70 ,GREEN);
    }
}

void WIN_FUN(float *Index, int n)
{
    // формулы взяты с сайта http://dmilvdv.narod.ru/SpeechSynthesis/index.html?window_function.html
    // вычисляем коэффициенты оконной функции
    int i;
    double a;
    for (i=0; i<n ; i++)
    {
        a = M_PI / n * (1.0 + 2.0 * i);
        Index[i]=0.5 * (1.0 - 0.16 - cos(a) + 0.16 * cos(2.0 * a));
    }
    return;
}



void DrawWIN(float* sampleBlock)
{
    int x=0 , FirstZero=0;
    // первое ненулевое слева
    for(x=1; x<FRAMES_PER_BUFFER; x++)// со второго значения
    {
        if (sampleBlock[x]>=0 && sampleBlock[x-1]<0)
        {
            FirstZero=x;
            break;
        }
    }

    for( x=FirstZero; x<FRAMES_PER_BUFFER; x++)
    {
        DrawPixel(screen, 50 + (x-FirstZero)/2.  , AXIS_ZERO_WIN - sampleBlock[x]*70 , RED);
    }
}

void DrawFFT(float* sampleBlock)
{
    int x=0, i=0, max_i=0, y=0 , max_y;
    float max_ampl;
    float PowFreq[FRAMES_PER_BUFFER];//для спектра
    float Im[FRAMES_PER_BUFFER]; // мнимая часть
    CLEAR( Im );//обнуляем
    // преобразование фурье
    FFT(sampleBlock, Im, FRAMES_PER_BUFFER, POW_2, -1); // отправляем на фурье
    // вычисление значений магнитуд и максимальное
    max_ampl=0;
    max_i=0;
    max_y=0;
    for(i=0; i<FRAMES_PER_BUFFER / 2; i++)
    {
        PowFreq[i]=sqrt (sampleBlock[i]*sampleBlock[i]+Im[i]*Im[i] );
        if (PowFreq[i]>max_ampl)
        {
            max_ampl=PowFreq[i]; // максимальное значение
            max_i=i;             // и его индекс
        }
    }

    // отрисовка спектра

    for( x=0; x<FRAMES_PER_BUFFER / 20 ; x++)
    {
        max_y=PowFreq[x];
        if (PowFreq[x]>250) max_y=250;
        for (y=0 ; y<= max_y *2; y++)
        {
            DrawPixel(screen, 50 + x*10, AXIS_ZERO_FFT - y ,BROUN);
        }
    }

    // интерполяция, определяем вершину параболы по 3 точкам для точного значения частоты
    VerPar((float)max_i-1,
           PowFreq[max_i-1] ,
           (float)max_i,
           max_ampl ,
           (float)max_i+1,
           PowFreq[max_i+1] );
    // рисуем точку максимума
    max_y=Y0;
    if (Y0>250) max_y=250;
    for (y=0 ; y<= max_y *2; y++)
    {
        DrawPixel(screen, 50 + X0*10, AXIS_ZERO_FFT - y ,250,120,0);
        DrawPixel(screen, 51 + X0*10, AXIS_ZERO_FFT - y ,250,120,0);
    }

    Freq=X0*SAMPLE_RATE/FRAMES_PER_BUFFER; // вершина параболы в частоту
}

void DrawText()
{
    SDL_BlitSurface(sound_txt, NULL, screen, &sound_pos);
    SDL_BlitSurface(sma_txt, NULL, screen, &sma_pos);
    SDL_BlitSurface(win_txt, NULL, screen, &win_pos);
    SDL_BlitSurface(fft_txt, NULL, screen, &fft_pos);

    // числа на спектре
    if (Freq > 50)
    {

        char temp_text[50];
        // частота
        sprintf(temp_text, "Freq=%.2f", Freq);
        SDL_Color orange = {ORANGE};
        freq_txt = TTF_RenderUTF8_Blended(my_font, temp_text, orange);
        freq_pos.x = power_pos.x = 52 + X0 * 10 ;
        SDL_BlitSurface(freq_txt, NULL, screen, &freq_pos);
        // значение
        sprintf(temp_text, "Power=%.1f", Y0);
        power_txt = TTF_RenderUTF8_Blended(my_font, temp_text, orange);
        SDL_BlitSurface(power_txt, NULL, screen, &power_pos);

    }


}

void Slock(SDL_Surface *screen)
{

    if ( SDL_MUSTLOCK(screen) )
    {
        if ( SDL_LockSurface(screen) < 0 )
        {
            return;
        }
    }

}

/* ------------------------------------------------------ */
void Sulock(SDL_Surface *screen)
{

    if ( SDL_MUSTLOCK(screen) )
    {
        SDL_UnlockSurface(screen);
    }

}

int  FFT(float *Rdat, float *Idat, int N, int LogN, int Ft_Flag)
{
    // parameters error check:
    if((Rdat == NULL) || (Idat == NULL))                  return 0;
    if((N > 16384) || (N < 1))                            return 0;
    if(!NUMBER_IS_2_POW_K(N))                             return 0;
    if((LogN < 2) || (LogN > 14))                         return 0;
    if((Ft_Flag != FT_DIRECT) && (Ft_Flag != FT_INVERSE)) return 0;

    register int  i, j, n, k, io, ie, in, nn;
    float         ru, iu, rtp, itp, rtq, itq, rw, iw, sr;

    static const float Rcoef[14] =
    {
        -1.0000000000000000F,  0.0000000000000000F,  0.7071067811865475F,
        0.9238795325112867F,  0.9807852804032304F,  0.9951847266721969F,
        0.9987954562051724F,  0.9996988186962042F,  0.9999247018391445F,
        0.9999811752826011F,  0.9999952938095761F,  0.9999988234517018F,
        0.9999997058628822F,  0.9999999264657178F
    };
    static const float Icoef[14] =
    {
        0.0000000000000000F, -1.0000000000000000F, -0.7071067811865474F,
        -0.3826834323650897F, -0.1950903220161282F, -0.0980171403295606F,
        -0.0490676743274180F, -0.0245412285229122F, -0.0122715382857199F,
        -0.0061358846491544F, -0.0030679567629659F, -0.0015339801862847F,
        -0.0007669903187427F, -0.0003834951875714F
    };

    nn = N >> 1;
    ie = N;
    for(n=1; n<=LogN; n++)
    {
        rw = Rcoef[LogN - n];
        iw = Icoef[LogN - n];
        if(Ft_Flag == FT_INVERSE) iw = -iw;
        in = ie >> 1;
        ru = 1.0F;
        iu = 0.0F;
        for(j=0; j<in; j++)
        {
            for(i=j; i<N; i+=ie)
            {
                io       = i + in;
                rtp      = Rdat[i]  + Rdat[io];
                itp      = Idat[i]  + Idat[io];
                rtq      = Rdat[i]  - Rdat[io];
                itq      = Idat[i]  - Idat[io];
                Rdat[io] = rtq * ru - itq * iu;
                Idat[io] = itq * ru + rtq * iu;
                Rdat[i]  = rtp;
                Idat[i]  = itp;
            }

            sr = ru;
            ru = ru * rw - iu * iw;
            iu = iu * rw + sr * iw;
        }

        ie >>= 1;
    }

    for(j=i=1; i<N; i++)
    {
        if(i < j)
        {
            io       = i - 1;
            in       = j - 1;
            rtp      = Rdat[in];
            itp      = Idat[in];
            Rdat[in] = Rdat[io];
            Idat[in] = Idat[io];
            Rdat[io] = rtp;
            Idat[io] = itp;
        }

        k = nn;

        while(k < j)
        {
            j   = j - k;
            k >>= 1;
        }

        j = j + k;
    }

    if(Ft_Flag == FT_DIRECT) return 1;

    rw = 1.0F / N;

    for(i=0; i<N; i++)
    {
        Rdat[i] *= rw;
        Idat[i] *= rw;
    }

    return 1;
}

void VerPar(float x1,float y1,float x2,float y2,float x3,float y3)
{
    // определяем координаты вершины параболы, выводим в глобал переменные
    float A,B,C;
    A=(y3-( (x3*(y2-y1)+x2*y1-x1*y2) /  (x2-x1)))   /(x3*(x3-x1-x2)+x1*x2);
    B= (y2-y1)/(x2-x1) - A*(x1+x2);
    C= ( (x2*y1-x1*y2) / (x2-x1) )+A*x1*x2;
    X0= -(B/(2*A));
    Y0= - ( (B*B-4*A*C)  /   (4*A) );
    return;
}
