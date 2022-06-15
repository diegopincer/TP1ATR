// Main.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//

//  Automação em tempo Real - ELT012 - UFMG
//  
//  EXEMPLO 8 - Sincronismo entre threads via Variáveis de Condição na API Win32
//  ----------------------------------------------------------------------------
//
//  Versão 1.0 - 22/04/2010 - Prof. Luiz T. S. Mendes
//
//  NOTA: Este programa é funcionalmente idêntico ao Exemplo 7, com a única
//  diferença de que as chamadas referentes à criação de threads e variáveis de
//  condição foram portadas para a API Win32.
//
//	Este programa exemplifica o uso de variáveis de condição para o sincronismo
//  entre threads. Para tal, a thread primária do programa cria 5 threads
//  secundárias, que ficam bloqueadas em 3 eventos distintos:
//
//    thread secundária   A       - desbloqueia-se quando as teclas "A" ou ESC
//                                  forem digitadas no teclado
//    thread secundária   B       - idem, referente às teclas "B" ou ESC
//    threads secundárias C, D, E - desbloqueiam-se ao mesmo tempo quando a
//                                  tecla de espaço ou ESC forem acionados.
//
//  Estes eventos são modelados como variáveis de condição. A tecla ESC,
//  quando digitada, sinaliza o término de execução para todas as threads.
//

#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#include <windows.h>
#include <process.h>	// _beginthreadex() e _endthreadex()
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>		// _isdigit	

SYSTEMTIME tempo;

#define NTHREADS	5
#define	ESC			0x1B
#define TeclaA      0x61
#define TeclaB      0x62
#define TeclaEsp	0x20
#define Sisot 11//TIPO DA MENSAGEM
#define DadsProcesso 22
#define MensAlar 55


// Casting para terceiro e sexto parâmetros da função _beginthreadex
typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

// DECLARACAO DOS PROTOTIPOS DE FUNCAO CORRESPONDENTES ÀS THREADS SECUNDARIAS
DWORD WINAPI ThreadFunc(LPVOID);	// declaração da função


// VARIAVEIS GLOBAIS
long contador = 0;   // Variavel incrementada continuamente pelas threads
int  tecla = 0;      // Caracter digitado no teclado
int cont = 0;//Variavel contadore de mensagens

//Variaveis para dados do sistema de otimizaçao
float SP_PRESS = 0;
float SP_TEMP = 0;
int VOL = 0;

//Variaveis dados do processo 
float PRESSAO_T = 0;
float TEMP = 0;
float PRESSAO_G = 0;
float NIVEL = 0;

//Variaveis para  mensagens de alarmes 
int ID = 0;
int PRIORIDADE = 0;

// Declaração das variáveis de condição e CRITICAL SECTION
CONDITION_VARIABLE CondVarA, CondVarB, CondVarEsp;
CRITICAL_SECTION CritSec;


struct thread_data {
	int tecla;
	int index;
	CONDITION_VARIABLE* condvar;
};

// THREAD PRIMARIA
int main()
{
	/*while (1) {
		if (cont == 1000000)cont = 0;//Nao é o ideal, masa temporariamente vai servir 
		//gerador de mensagem do
		GetLocalTime(&tempo);
		SP_PRESS = rand() % 10000;
		SP_TEMP = rand() % 10000;
		VOL= rand() % 100000;
		printf("SISTEMA DE OTIMIZACAO:%06d|%d|%06.1f|%06.1f|%05d|%02d:%02d:%02d\n",cont, Sisot,SP_PRESS,SP_TEMP,VOL, tempo.wHour,tempo.wMinute, tempo.wSecond);//para garantir que um numero vai ser impreso com apenas uma casa decimal deve %0.1f, mas temos que  olhar o 
		cont++;
		Sleep(500);
		PRESSAO_T = rand() % 10000;
		TEMP = rand() % 10000;
		PRESSAO_G = rand() % 10000;
		NIVEL = rand() % 10000;
		printf("DADOS DE PROCESSO:    %06d|%d|%06.1f|%06.1f|%06.1f|%06.1f|%02d:%02d:%02d\n", cont, DadsProcesso, PRESSAO_T, TEMP, PRESSAO_G, NIVEL, tempo.wHour, tempo.wMinute, tempo.wSecond);
		cont++;
		Sleep(500);
		ID = rand() % 10000;
		PRIORIDADE = rand() % 1000; cont,
		printf("MENSAGENS DE ALARME:  %06d|%d|%04d|%03d|%02d:%02d:%02d\n", cont, MensAlar, ID, PRIORIDADE, tempo.wHour, tempo.wMinute, tempo.wSecond);
		cont++;
		Sleep(500);
	}*/
	HANDLE hThreads[NTHREADS];
	int i;
	DWORD status, dwThreadID, dwExitCode;
	int teclas[5] = { TeclaA, TeclaB, TeclaEsp, TeclaEsp, TeclaEsp };
	CONDITION_VARIABLE* condvars[5] = { &CondVarA, &CondVarB, &CondVarEsp, &CondVarEsp, &CondVarEsp };
	thread_data parametros[5];
	BOOL StatusMemCirc, StatusAlarmes, StatusDados, StatusOtimizacao;
	STARTUPINFO si;
	PROCESS_INFORMATION NewProcess;

	//-----------------------------------------------------------------------------
	// Particularidade do Windows - Define o título da janela
	//-----------------------------------------------------------------------------

	//SetConsoleTitle("Exemplo 1 - Criando threads via Pthreads");

	//-----------------------------------------------------------------------------
	// Criação da CRITICAL SECTION
	//-----------------------------------------------------------------------------

	InitializeCriticalSection(&CritSec);

	//-----------------------------------------------------------------------------
	// Criação das variáveis de condição. São utilizados os atributos "default"
	// para estas variáveis.
	//-----------------------------------------------------------------------------

	InitializeConditionVariable(&CondVarA);
	InitializeConditionVariable(&CondVarB);
	InitializeConditionVariable(&CondVarEsp);

	//Criando Processo de memória Circular
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);	// Tamanho da estrutura em bytes
	
	StatusMemCirc = CreateProcess(
		"..\\Debug\\MemCirc.exe", // Nome
		NULL,	// linha de comando
		NULL,	// atributos de seguran�a: Processo
		NULL,	// atributos de seguran�a: Thread
		FALSE,	// heran�a de handles
		NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,	// CreationFlags
		NULL,	// lpEnvironment
		"..\\Debug",	// diretório corrente do filho
		&si,			// lpStartUpInfo
		&NewProcess);	// lpProcessInformation
		GetLastError();

		//Criando Processo de Tratamento de Alarmes
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);	// Tamanho da estrutura em bytes

		StatusAlarmes = CreateProcess(
			"..\\Debug\\ProcessoAlarmes.exe", // Nome
			NULL,	// linha de comando
			NULL,	// atributos de seguran�a: Processo
			NULL,	// atributos de seguran�a: Thread
			FALSE,	// heran�a de handles
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,	// CreationFlags
			NULL,	// lpEnvironment
			"..\\Debug",	// diretório corrente do filho
			&si,			// lpStartUpInfo
			&NewProcess);	// lpProcessInformation
		GetLastError();

		//Criando Processo de Tratamento de Dados
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);	// Tamanho da estrutura em bytes

		StatusDados = CreateProcess(
			"..\\Debug\\ProcessoDados.exe", // Nome
			NULL,	// linha de comando
			NULL,	// atributos de seguran�a: Processo
			NULL,	// atributos de seguran�a: Thread
			FALSE,	// heran�a de handles
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,	// CreationFlags
			NULL,	// lpEnvironment
			"..\\Debug",	// diretório corrente do filho
			&si,			// lpStartUpInfo
			&NewProcess);	// lpProcessInformation
		GetLastError();

		//Criando Processo de Otimização
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);	// Tamanho da estrutura em bytes

		StatusOtimizacao = CreateProcess(
			"..\\Debug\\ProcessoOtimizacao.exe", // Nome
			NULL,	// linha de comando
			NULL,	// atributos de seguran�a: Processo
			NULL,	// atributos de seguran�a: Thread
			FALSE,	// heran�a de handles
			NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,	// CreationFlags
			NULL,	// lpEnvironment
			"..\\Debug",	// diretório corrente do filho
			&si,			// lpStartUpInfo
			&NewProcess);	// lpProcessInformation
		GetLastError();

	//-----------------------------------------------------------------------------
	// Loop de criacao das threads
	//-----------------------------------------------------------------------------

	for (i = 0; i < 5; ++i) {	// cria 5 threads
		parametros[i].tecla = teclas[i];
		parametros[i].index = i;
		parametros[i].condvar = condvars[i];
		hThreads[i] = (HANDLE)_beginthreadex(
			NULL,
			0,
			(CAST_FUNCTION)ThreadFunc,	// casting necessário
			(void*)&parametros[i],
			0,
			(CAST_LPDWORD)&dwThreadID	// cating necessário
		);

		if (hThreads[i]) printf("Thread %d criada com Id= %0d \n", i, dwThreadID);
		else printf("Erro na criacao da thread %d! Codigo = %d\n", i, GetLastError());
	}	// for

	//-----------------------------------------------------------------------------
	// Aguarda usuario digitar a tecla ESC para encerrar
	//-----------------------------------------------------------------------------

	do {
		printf("Digite \"a\", \"b\", <Espaco> ou <Esc> para terminar:\n");
		tecla = _getch();

		if (tecla == TeclaA || tecla == ESC)
			WakeConditionVariable(&CondVarA);

		if (tecla == TeclaB || tecla == ESC)
			WakeConditionVariable(&CondVarB);

		if (tecla == TeclaEsp || tecla == ESC)
			WakeAllConditionVariable(&CondVarEsp);

	} while (tecla != ESC);

	//-----------------------------------------------------------------------------
	// Aguarda termino das threads e fecha seus handles
	//-----------------------------------------------------------------------------

	status = WaitForMultipleObjects(NTHREADS, hThreads, TRUE, INFINITE);
	if (status != WAIT_OBJECT_0) {
		printf("Erro em WaitForMultipleObjects! Codigo = %d\n", GetLastError());
		return 0;
	}

	for (i = 0; i < NTHREADS; ++i) {
		GetExitCodeThread(hThreads[i], &dwExitCode);
		printf("thread %d terminou: codigo=%d\n", i, dwExitCode);
		CloseHandle(hThreads[i]);	// apaga referência ao objeto
	}

	/* DESTRUIR VARIAVEIS DE CONDICAO */

	return 0;
}

// THREADS SECUNDARIAS
DWORD WINAPI ThreadFunc(LPVOID thread_arg) {
	int index;
	BOOL status;
	struct thread_data* parametros;
	CONDITION_VARIABLE* condvar;

	parametros = (struct thread_data*)thread_arg;
	index = parametros->index;
	condvar = parametros->condvar;

	do {
		// Devemos nos lembrar do processo de espera por uma variável de condição:
		// 1. Conquista-se a CRITICAL SECTION associada ao "predicado" a ser testado;
		// 2. while (PREDICADO NAO VERDADEIRO) {
		//      "Aguarda sinalizacao da variavel de condicao";
		//    }
		// 3. Libera CRITICAL SECTION associada à variável de condição.
		//
		EnterCriticalSection(&CritSec);
		while (tecla != parametros->tecla) {
			status = SleepConditionVariableCS(parametros->condvar, &CritSec, INFINITE);
			if (!status) {
				printf("Thread %d: Erro %d na espera da condicao!\n", index, GetLastError());
				exit(0);
			}
			printf("Thread %d: condicao sinalizada! tecla = \"%c\"\n", index, tecla);
			if (tecla == ESC) break;
		}

		if (tecla != ESC) tecla = 0;
		LeaveCriticalSection(&CritSec);
	} while (tecla != ESC);

	_endthreadex((DWORD)index);
	// O comando "return" abaixo é desnecessário, mas presente aqui para compatibilidade
	// com o Visual Studio da Microsoft
	return index;
}