#include<stdio.h>
#include<math.h>
#include <stdlib.h>
#include <time.h>
#include<string.h>
# define org_row  720 
# define org_col  480
# define sizebyte 1
# define in_file "football_bf.y"
# define pi 3.1415926535897
# define N 4//block 크기
# define use_psnr 1//(0 : psnr 측정 x   , 1 : psnr 측정 o ) 
# define FALSE 0
# define TRUE 1
//====option================
# define out_file "공간예측_decoding.y"
# define  reconstruct_file "공간예측_before복원.y"
# define label_file "label"
# define error_file "error"
# define sample 1.75 //양자화 계수
# define pre_N 9 // 예측 방향 (4 or 9)

//파일 읽기
unsigned char* ReadFile(char* s, int size_row, int size_col) {
	//FILE* input_img = fopen(in_file, "rb");
	FILE* input_img;
	fopen_s(&input_img, s, "rb");
	unsigned char* org_img = NULL;
	org_img = (unsigned char*)malloc(sizeof(unsigned char)*(size_row*size_col));
	if (input_img == NULL) {
		puts("input 파일 오픈 실패 ");
		return NULL;
	}
	else {
		fseek(input_img, 0, SEEK_SET);
		fread((void*)org_img, sizebyte, size_row*size_col, input_img);
	}
	return org_img;
	fclose(input_img);
	free(org_img);
}

//파일 읽기
int* ReadFile_int(char* s, int size_row, int size_col) {
	//FILE* input_img = fopen(in_file, "rb");
	FILE* input;
	int* org = NULL;

	fopen_s(&input, s, "rb");
	org = (int*)malloc(sizeof(int)*(org_row*org_col));
	if (input == NULL) {
		puts("input 파일 오픈 실패 ");
		return NULL;
	}
	else {
		fseek(input, 0, SEEK_SET);
		fread((void*)org, sizeof(int), org_row*org_col, input);
	}
	fclose(input);
	return org;
	free(org);
}

//unsigned char 형 파일 쓰기
unsigned char* WriteFile_U(unsigned char* out_img, char* s) {
	//FILE* output_img = fopen(out_file, "wb");
	FILE* output_img;
	fopen_s(&output_img, s, "wb");
	if (output_img == NULL) {
		puts("output 파일 오픈 실패");
		return NULL;
	}
	else {
		fseek(output_img, 0, SEEK_SET);
		fwrite((void*)out_img, sizebyte, org_row*org_col, output_img);
	}
	fclose(output_img);
	return out_img;

}

//MSE & PSNR **입력은 uchar 형**
double MSE_f(unsigned char* sp_img, char* s)
{
	double MSE;
	double PSNR;
	double sum = 0.0;
	int i, j;
	//FILE* in2 = fopen("lena.img", "rb");
	FILE* in2;
	fopen_s(&in2, s, "rb");
	unsigned char* org_img2 = NULL;
	org_img2 = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	fseek(in2, 0, SEEK_SET);
	fread((void*)org_img2, sizebyte, org_row*org_col, in2);
	for (i = 0; i < org_col; i++)
	{
		for (j = 0; j< org_row; j++) {
			double temp = (double)(*(org_img2 + i*org_row + j)) - (double)(*(sp_img + i*org_row + j));
			sum += pow(temp, 2); //sum += temp*temp
		}
	}
	MSE = (long double)sum / ((double)org_row*(double)org_col);
	PSNR = 10 * log10(255.0*255.0 / MSE);
	fseek(in2, 0, SEEK_SET);
	printf("\n\nMSE = %f\n\n", MSE);
	printf("\n\nPSNR = %f\n\n", PSNR);
	fclose(in2);
	free(org_img2);
	return MSE;
}

//(미확인)mismatch 확인
int ismismatch(unsigned char* en_img, unsigned char* de_img) {
	int i, j;
	for (i = 0; i < org_col; i++)
	{
		for (j = 0; j< org_row; j++) {
			if (*(en_img + i*org_row + j) != *(de_img + i*org_row + j)) {
				//printf("%d 열 %d 행 : mismatch %d \n", i, j, *(en_img + i*org_row + j) - *(de_img + i*org_row + j));//test 용===========
				return TRUE;
			}
		}
	}

	return FALSE;
}

//블록의 이웃한 픽셀 축력하기
unsigned char* neighbor_pixels(unsigned char* org_img, int i, int j) { // i = 행, j = 열
	int u, v;
	int w;
	unsigned char* neighbor_pix = NULL;
	neighbor_pix = (unsigned char*)malloc(sizeof(unsigned char)*(N + pre_N)); //8 or 13
	if (org_img == NULL) {
		for (w = 0; w < pre_N; w++) {
			*(neighbor_pix + w) = 128;
		}
	}

	else {
		//A,B,C,D
		for (u = 0; u < N; u++) {
			if ((j - 1) >= 0)
				*(neighbor_pix + u) = *(org_img + (j - 1)*org_row + i + u);
			else
				*(neighbor_pix + u) = 128;
		}

		//I,J,K,L
		for (v = 0; v < N; v++) {
			if ((i - 1) >= 0)
				*(neighbor_pix + N + v) = *(org_img + (j + v)*org_row + i - 1);
			else
				*(neighbor_pix + N + v) = 128;
		}
		if (pre_N == 9) {
			//E,F,G,H
			if (i + N + 3 <= org_row - 1) {
				for (u = 0; u < N; u++) {
					if ((j - 1) >= 0)
						*(neighbor_pix + 2 * N + u) = *(org_img + (j - 1)*org_row + i + u + N);
					else
						*(neighbor_pix + 2 * N + u) = 128;
				}
			}
			else {
				for (u = 0; u < N; u++) {
					if ((j - 1) >= 0)
						*(neighbor_pix + 2 * N + u) = *(org_img + (j - 1)*org_row + i + 3);
					else
						*(neighbor_pix + 2 * N + u) = 128;
				}
			}
			//Q
			if ((j - 1) >= 0 && (i - 1) >= 0)
				*(neighbor_pix + 3 * N) = *(org_img + (j - 1)*org_row + i - 1);
			else
				*(neighbor_pix + 3 * N) = 128;
		}
	}
	return  neighbor_pix;
	free(neighbor_pix);
}

//예측 블록 출력
unsigned char* pre_block(unsigned char* neighbor_pix, int type, int row, int col) {
	unsigned char* block = NULL;

	int i, u, v;
	unsigned char m;
	double sum = 0;

	int A = *(neighbor_pix);
	int B = *(neighbor_pix + 1);
	int C = *(neighbor_pix + 2);
	int D = *(neighbor_pix + 3);
	int I = *(neighbor_pix + 4);
	int J = *(neighbor_pix + 5);
	int K = *(neighbor_pix + 6);
	int L = *(neighbor_pix + 7);
	int E, F, G, H, Q;
	if (pre_N == 9) {
		E = *(neighbor_pix + 8);
		F = *(neighbor_pix + 9);
		G = *(neighbor_pix + 10);
		H = *(neighbor_pix + 11);
		Q = *(neighbor_pix + 12);
	}

	block = (unsigned char*)malloc(sizeof(unsigned char)*(N*N));

	if (type == 0) {
		for (v = 0; v < N; v++) {
			for (u = 0; u < N; u++) {
				*(block + v*N + u) = *(neighbor_pix + u);
			}
		}
	}
	else if (type == 1) {
		for (v = 0; v < N; v++) {
			for (u = 0; u < N; u++) {
				*(block + v*N + u) = *(neighbor_pix + v + N);
			}
		}
	}

	else if (type == 2) {
		sum = 0;
		if (row - 1 >= 0 && col - 1 >= 0) {
			for (i = 0; i < 2 * N; i++) {
				sum += *(neighbor_pix + i);
			}
			m = (unsigned char)(sum / ((double)N*2.0) + 0.5);
		}
		else if (row - 1 < 0) { // 세로 막대 = 128
			for (i = 0; i < N; i++) {
				sum += *(neighbor_pix + i);
			}
			m = (unsigned char)(sum / ((double)N) + 0.5);
		}
		else if (col - 1 < 0) { // 가로 막대 = 128
			for (i = 0; i < N; i++) {
				sum += *(neighbor_pix + N + i);
			}
			m = (unsigned char)(sum / ((double)N) + 0.5);
		}
		else
			m = 128;

		for (v = 0; v < N; v++) {
			for (u = 0; u < N; u++) {
				*(block + v*N + u) = m;
			}
		}
	}

	else if (type == 3) {
		if (pre_N == 4) {
			for (i = 0; i < N*N; i++) {
				if (i % (N + 1) == 0)
					*(block + i) = (A + I + 1) >> 1;
				else if (i % (N + 1) == 1)
					*(block + i) = (A + B + 1) >> 1;
				else if (i % (N + 1) == 2 && i < N * 2)
					*(block + i) = (B + C + 1) >> 1;
				else if (i % (N + 1) == 3 && i < N * 2)
					*(block + i) = (C + D + 1) >> 1;
				else if (i % (N + 1) == 4)
					*(block + i) = (I + J + 1) >> 1;
				else if (i % (N + 1) == 3 && i > N * 2)
					*(block + i) = (J + K + 1) >> 1;
				else if (i % (N + 1) == 2 && i > N * 2)
					*(block + i) = (K + L + 1) >> 1;
			}
		}
		else if (pre_N == 9) {
			for (i = 0; i < N*N; i++) {
				if (i == 0)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 1 || i == 4)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 2 || i == 5 || i == 8)
					*(block + i) = (C + 2 * D + E + 2) >> 2;
				else if (i == 3 || i == 6 || i == 9 || i == 12)
					*(block + i) = (D + 2 * E + F + 2) >> 2;
				else if (i == 7 || i == 10 || i == 13)
					*(block + i) = (E + 2 * F + G + 2) >> 2;
				else if (i == 11 || i == 14)
					*(block + i) = (F + 2 * G + H + 2) >> 2;
				else if (i == 15)
					*(block + i) = (G + 3 * H + 2) >> 2;
			}
		}
	}
	if (pre_N == 9) {
		if (type == 4) {
			for (i = 0; i < N*N; i++) {
				if (i % (N + 1) == 0)
					*(block + i) = (A + 2 * Q + I + 2) >> 2;
				else if (i % (N + 1) == 1)
					*(block + i) = (Q + 2 * A + B + 2) >> 2;
				else if (i % (N + 1) == 2 && i < N * 2)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i % (N + 1) == 3 && i < N * 2)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i % (N + 1) == 4)
					*(block + i) = (Q + 2 * I + J + 2) >> 2;
				else if (i % (N + 1) == 3 && i > N * 2)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
				else if (i % (N + 1) == 2 && i > N * 2)
					*(block + i) = (J + 2 * K + L + 2) >> 2;
			}
		}
		else if (type == 5) {
			for (i = 0; i < N*N; i++) {
				if (i == 0)
					*(block + i) = (A + B + 1) >> 1;
				else if (i == 1 || i == 8)
					*(block + i) = (B + C + 1) >> 1;
				else if (i == 2 || i == 9)
					*(block + i) = (C + D + 1) >> 1;
				else if (i == 3 || i == 10)
					*(block + i) = (D + E + 1) >> 1;
				else if (i == 11)
					*(block + i) = (E + F + 1) >> 1;
				else if (i == 4)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 5 || i == 12)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 6 || i == 13)
					*(block + i) = (C + 2 * D + E + 2) >> 2;
				else if (i == 7 || i == 14)
					*(block + i) = (D + 2 * E + F + 2) >> 2;
				else if (i == 15)
					*(block + i) = (E + 2 * F + G + 2) >> 2;
			}
		}
		else if (type == 6) {
			for (i = 0; i < N*N; i++) {
				if (i == 0 || i == 6)
					*(block + i) = (Q + I + 1) >> 1;
				else if (i == 1 || i == 7)
					*(block + i) = (I + 2 * Q + A + 2) >> 2;
				else if (i == 2)
					*(block + i) = (Q + 2 * A + B + 2) >> 2;
				else if (i == 3)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 4 || i == 10)
					*(block + i) = (I + J + 1) >> 1;
				else if (i == 5 || i == 11)
					*(block + i) = (Q + 2 * I + J + 2) >> 2;
				else if (i == 8 || i == 14)
					*(block + i) = (J + K + 1) >> 1;
				else if (i == 9 || i == 15)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
				else if (i == 12)
					*(block + i) = (K + L + 1) >> 1;
				else if (i == 13)
					*(block + i) = (J + 2 * K + L + 2) >> 2;
			}
		}
		else if (type == 7) {
			for (i = 0; i < N*N; i++) {
				if (i == 0 || i == 9)
					*(block + i) = (Q + A + 1) >> 1;
				else if (i == 1 || i == 10)
					*(block + i) = (A + B + 1) >> 1;
				else if (i == 2 || i == 11)
					*(block + i) = (B + C + 1) >> 1;
				else if (i == 3)
					*(block + i) = (C + D + 1) >> 1;
				else if (i == 4 || i == 13)
					*(block + i) = (I + 2 * Q + A + 2) >> 2; //****************************
				else if (i == 5 || i == 14)
					*(block + i) = (Q + 2 * A + B + 2) >> 2;
				else if (i == 6 || i == 15)
					*(block + i) = (A + 2 * B + C + 2) >> 2;
				else if (i == 7)
					*(block + i) = (B + 2 * C + D + 2) >> 2;
				else if (i == 8)
					*(block + i) = (Q + 2 * I + J + 2) >> 2;
				else if (i == 12)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
			}
		}
		else if (type == 8) {
			for (i = 0; i < N*N; i++) {
				if (i == 0)
					*(block + i) = (I + J + 1) >> 1;
				else if (i == 1)
					*(block + i) = (I + 2 * J + K + 2) >> 2;
				else if (i == 2 || i == 4)
					*(block + i) = (J + K + 1) >> 1;
				else if (i == 3 || i == 5)
					*(block + i) = (J + 2 * K + L + 2) >> 2;
				else if (i == 6 || i == 8)
					*(block + i) = (K + L + 1) >> 1;
				else if (i == 7 || i == 9)
					*(block + i) = (K + 3 * L + 2) >> 2;
				else if (i >= 10)
					*(block + i) = L;
			}
		}
	}
	return block;
	free(block);
}

//(미확인) error sampling, 양자화 계수를 나눠줌
int* sampling_error(int* error, int type, int size_row, int size_col) {
	int n;
	int* error_d = NULL;
	error_d = (int*)malloc(sizeof(int)*(size_row*size_col));
	//type 이 0이면 error 를 양자화만 함
	if (type == 0) {
		for (n = 0; n < size_row*size_col; n++) {
			*(error_d + n) = (int)((double)(*(error + n)) / sample);
		}
		return error_d;
	}

	//type = 1 양자화된 에러를 다시 복구함 
	else if (type == 1) {
		for (n = 0; n < size_row*size_col; n++) {
			//*(error_d + n) = (int)((double)(*(error + n))*sample + 0.5);
			
			if ((*(error + n)) > 0) {
			*(error_d + n) = (int)((double)(*(error + n))*sample + 0.5);
			}
			else if ((*(error + n)) < 0) {
				*(error_d + n) = (int)((double)(*(error + n))*sample - 0.5);
			}
			else
				*(error_d + n) = 0;
			//*(error + n) = (*(error + n))*sample;
		}
		return error_d;
	}
	return NULL;
}

void sort_Label(unsigned char* Label_arr) {

	int* label_N = NULL;
	int i, j, w;

	label_N = (int*)malloc(sizeof(int)*(pre_N));
	for (w = 0; w < pre_N; w++) {
		*(label_N + w) = 0;
	}

	for (i = 0; i < org_col / N; i++) {
		for (j = 0; j < org_row / N; j++) {
			for (w = 0; w < pre_N; w++) {
				if (*(Label_arr + i*(org_row / N) + j) == w) {
					*(label_N + w) += 1;
				}
			}
		}
	}
	printf("\n");
	for (w = 0; w < pre_N; w++) {
		printf("number of %d label :  %d\n", w, *(label_N + w));
	}

	free(label_N);
}

void sort_Error(int* Error) {

	int* error_N = NULL;
	int i, j, w;

	error_N = (int*)malloc(sizeof(int)*(256 * 2));
	for (w = 0; w < 256 * 2; w++) {
		*(error_N + w) = 0;
	}

	for (i = 0; i < org_col; i++) {
		for (j = 0; j < org_row; j++) {
			for (w = 0; w < 2 * 256; w++) {
				if (*(Error + i*(org_row / N) + j) == w - 255) {
					*(error_N + w) += 1;
				}
			}
		}
	}
	printf("\n");
	for (w = -15 + 255; w < 15 + 255; w++) {
		printf("number of %d error :  %d\n", w - 255, *(error_N + w));
	}

	free(error_N);
}

//양자화된 error와 label 을 이용하여, decoding 하기
unsigned char* decoding(unsigned char* Label_arr, int* Error) {
	unsigned char* res_img = NULL; //복구 영상
	unsigned char* neighbor_pix = NULL;
	unsigned char* block_pre = NULL;

	int i, j;
	int u, v;
	unsigned char label;

	
	res_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_col*org_row));
	neighbor_pix = (unsigned char*)malloc(sizeof(unsigned char)*(N + pre_N)); //8 or 13
	block_pre = (unsigned char*)malloc(sizeof(unsigned char)*(N*N));
	
	//sort_Error(Error); //test 용==========================

	Error = sampling_error(Error, 1, org_row, org_col); //양자화된 error 복구
	//sort_Error(Error); //test 용==========================
	for (i = 0; i < org_col; i += N) {
		for (j = 0; j < org_row; j += N) {
			neighbor_pix = neighbor_pixels(res_img, j, i);
			label = *(Label_arr + (i / N)*(org_row / N) + j / N);
			block_pre = pre_block(neighbor_pix, label, j, i);
			for (v = 0; v < N; v++) {
				for (u = 0; u < N; u++) {
					int temp = *(block_pre + v*N + u)+ *(Error + (i + v)*org_row + j + u);
					if (temp < 0)
						*(res_img + (i + v)*org_row + j + u) = 0;
					else if (temp > 255)
						*(res_img + (i + v)*org_row + j + u) = 255;
					else
						*(res_img + (i + v)*org_row + j + u) = temp;
				}
			}
			/*
			//====================================================================
			if (180 <i < 200, 120 <j < 130)
				printf("%d\t", *(Error + (i + v)*org_row + (j + u)));//test용================
			//====================================================================
			*/
		}
	}
	//예측 영상 찾기
	return(res_img);
	free(res_img);
	free(neighbor_pix);
	free(block_pre);
}

//(미확인)main 함수
int main(void) {
	unsigned char* buff_img = NULL;
	unsigned char* encoding_img = NULL;
	unsigned char* decoding_img = NULL;
	unsigned char* Label_arr = NULL;
	int* Error = NULL;
	int mismatch;
	clock_t before, after;
	double Time;
	buff_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	encoding_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	decoding_img = (unsigned char*)malloc(sizeof(unsigned char)*(org_row*org_col));
	Label_arr = (unsigned char*)malloc(sizeof(unsigned char)*(org_col*org_row / (N*N)));
	Error = (int*)malloc(sizeof(int)*(org_col*org_row));
	
	before = clock();
	Label_arr = ReadFile(label_file, org_row/N, org_col/N);
	Error = ReadFile_int(error_file, org_row, org_col);
	
	sort_Label(Label_arr); // test 용 =======================
	//sort_Error(Error); //test 용==========================
	buff_img = decoding(Label_arr, Error); // decoding : error 와 label 을 받아 복구 영상 출력
	//sort_Label(Label_arr); // test 용 =======================
	//sort_Error(Error); //test 용==========================
	
	WriteFile_U(buff_img, out_file); //복원 영상 출력
	after = clock();
	Time = (double)(after - before);

	

	encoding_img = ReadFile(reconstruct_file, org_row, org_col);
	decoding_img = ReadFile(out_file, org_row, org_col);

	if (use_psnr == 1) {
		MSE_f(buff_img, in_file); // PSNR, MSE 계산, in_file과 비교
	}

	mismatch = ismismatch(encoding_img, decoding_img);
	if (mismatch == FALSE) {
		printf("\nmismatch 없음\n");
	}
	else
		printf("\nmismatch 발생\n");
	printf("\ntime : %.4f (sec)\n", Time / 1000);
	free(buff_img);
	free(encoding_img);
	free(decoding_img);
	free(Label_arr);
	free(Error);
	return 0;
}