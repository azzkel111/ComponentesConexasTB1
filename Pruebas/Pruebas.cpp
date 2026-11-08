// ============================================================
//  Pruebas.cpp  -  Pruebas automaticas de los algoritmos.
//  Compilar desde "Developer Command Prompt for VS 2022":
//     cl /clr /EHa /utf-8 /nologo /I..\ComponentesConexas Pruebas.cpp
//  y ejecutar Pruebas.exe. Devuelve 0 si todas las pruebas pasan.
//  Opciones: --traza (pasos del ejemplo) y --estadisticas.
// ============================================================
#include "Algoritmo.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace ComponentesConexas;

static int fallos = 0;
static long long pruebas = 0;

static void Check(bool cond, String^ msg) {
	pruebas++;
	if (!cond) { fallos++; if (fallos <= 20) Console::WriteLine("  FALLO: " + msg); }
}

static String^ Desc(Grafo^ g) {
	String^ s = "n=" + g->N.ToString() + " arcos:";
	for (int i = 0; i < g->N; i++)
		for (int j = 0; j < g->N; j++)
			if (g->Ady(i, j)) s += " " + (i + 1).ToString() + ">" + (j + 1).ToString();
	return s;
}

static Grafo^ Arcos(int n, array<int>^ pares) {   // pares 1-based: o1, d1, o2, d2, ...
	Grafo^ g = gcnew Grafo(n, true);
	for (int k = 0; k + 1 < pares->Length; k += 2) g->PonerArista(pares[k] - 1, pares[k + 1] - 1);
	return g;
}

// Alcanzabilidad por fuerza bruta (incluye camino de longitud 0).
static array<bool, 2>^ Alcance(Grafo^ g) {
	int n = g->N;
	array<bool, 2>^ a = gcnew array<bool, 2>(n, n);
	for (int s = 0; s < n; s++) {
		Stack<int>^ st = gcnew Stack<int>();
		st->Push(s); a[s, s] = true;
		while (st->Count > 0) {
			int u = st->Pop();
			for (int v = 0; v < n; v++) if (g->Ady(u, v) && !a[s, v]) { a[s, v] = true; st->Push(v); }
		}
	}
	return a;
}

// Verifica resultados e invariantes de los pasos para un grafo.
static void Verificar(Grafo^ g) {
	int n = g->N;
	List<List<int>^>^ ref = Algoritmos::Referencia(g);
	array<bool, 2>^ alc = Alcance(g);
	Resultado^ w = Algoritmos::Matricial(g);
	Resultado^ d = Algoritmos::Dfs(g);
	Check(Algoritmos::Iguales(Algoritmos::Canonico(w->componentes), ref), "Warshall distinto de referencia: " + Desc(g));
	Check(Algoritmos::Iguales(Algoritmos::Canonico(d->componentes), ref), "DFS distinto de referencia: " + Desc(g));
	Check(Algoritmos::Iguales(w->componentes, d->componentes), "los dos metodos dan bloques distintos: " + Desc(g));

	for each (Resultado ^ r in gcnew array<Resultado^>{ w, d }) {
		int k = r->componentes->Count;
		// Cantidad de pasos: inicio + diagonal + n (paso 2) + conteo + filas + columnas + k bloques + fin.
		Check(r->pasos->Count == n + k + 6, "cantidad de pasos");
		Check(r->pasos[0]->tipo == TipoPaso::Inicio && r->pasos[1]->tipo == TipoPaso::Diagonal, "pasos iniciales");
		Check(r->pasos[r->pasos->Count - 1]->tipo == TipoPaso::Fin, "el ultimo paso no es Fin");
		// Paso 1: M = A + I exacta.
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++) {
				Check(r->pasos[0]->matriz[i, j] == g->Ady(i, j), "el paso inicial no muestra A");
				Check(r->pasos[1]->matriz[i, j] == (i == j ? 1 : g->Ady(i, j)), "M = A + I incorrecta");
			}
		// Paso 2: la matriz de caminos coincide con la alcanzabilidad real.
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				Check((r->caminos[i, j] == 1) == alc[i, j], "matriz de caminos incorrecta: " + Desc(g));
		// Durante el paso 2 solo se agregan unos (nunca se borran).
		for (int t = 1; t + 1 < r->pasos->Count; t++)
			for (int i = 0; i < n; i++)
				for (int j = 0; j < n; j++)
					Check(r->pasos[t + 1]->matriz[i, j] >= r->pasos[t]->matriz[i, j], "un 1 desaparecio");
		// Paso 3: filas ordenadas de mayor a menor cantidad de 1; empate: el 1 mas a la izquierda.
		array<int>^ o = r->orden;
		for (int p = 0; p + 1 < n; p++) {
			int ca = Algoritmos::ContarUnos(r->caminos, o[p]), cb = Algoritmos::ContarUnos(r->caminos, o[p + 1]);
			Check(ca >= cb, "orden de filas no decreciente");
			if (ca == cb) {
				int pa = 0, pb = 0;
				while (r->caminos[o[p], pa] == 0) pa++;
				while (r->caminos[o[p + 1], pb] == 0) pb++;
				Check(pa <= pb, "empate mal resuelto (primer 1 mas a la izquierda)");
			}
		}
		// Paso 4: cada componente es un bloque cuadrado diagonal de 1 en posiciones consecutivas.
		int pos = 0;
		for (int c = 0; c < k; c++) {
			int tam = r->componentes[c]->Count;
			for (int a = pos; a < pos + tam; a++) {
				Check(r->comp[o[a]] == c, "bloque no consecutivo");
				for (int b = pos; b < pos + tam; b++) Check(r->caminos[o[a], o[b]] == 1, "el bloque diagonal tiene un 0");
			}
			pos += tam;
		}
		Check(pos == n, "los bloques no cubren todos los nodos");
		// Cada nodo en exactamente una componente, y la componente no cambia una vez asignada.
		array<int>^ visto = gcnew array<int>(n);
		for (int v = 0; v < n; v++) visto[v] = -1;
		for each (Paso ^ p in r->pasos) {
			for (int v = 0; v < n; v++) {
				if (visto[v] != -1) Check(p->comp[v] == visto[v], "comp cambia despues de asignarse");
				if (p->comp[v] != -1) visto[v] = p->comp[v];
			}
			Check(p->titulo != nullptr && p->texto != nullptr && p->texto->Length > 0, "paso sin texto");
		}
		for (int v = 0; v < n; v++) Check(r->comp[v] >= 0, "nodo sin componente");
	}
}

static void Esperar(Grafo^ g, array<String^>^ esperadas, String^ nombre) {
	Resultado^ r = Algoritmos::Matricial(g);
	bool ok = r->componentes->Count == esperadas->Length;
	for (int k = 0; ok && k < esperadas->Length; k++) ok = Algoritmos::Conjunto(r->componentes[k]) == esperadas[k];
	Check(ok, nombre);
	Verificar(g);
}

int main(array<String^>^ args) {
	// Ronda 1: validacion de entradas del modelo
	Console::WriteLine("Ronda 1: validacion de entradas del grafo");
	for each (int malo in gcnew array<int>{ -1, 0, 3, 13, 100 }) {
		bool lanzo = false;
		try { gcnew Grafo(malo, true); }
		catch (ArgumentOutOfRangeException^) { lanzo = true; }
		Check(lanzo, "Grafo(" + malo.ToString() + ") deberia lanzar excepcion");
	}
	Grafo^ g4 = gcnew Grafo(4, true);
	g4->AlternarArista(2, 2);  Check(g4->Ady(2, 2) == 1 && g4->ContarLazos() == 1, "no se pudo crear un lazo");
	g4->AlternarArista(2, 2);  Check(g4->Ady(2, 2) == 0, "no se pudo quitar el lazo");
	g4->AlternarArista(-1, 2); g4->AlternarArista(0, 9);
	Check(g4->ContarAristas() == 0, "arco fuera de rango aceptado");
	g4->AlternarArista(0, 1);  Check(g4->Ady(0, 1) == 1 && g4->Ady(1, 0) == 0, "el arco dirigido se duplico");
	g4->AlternarArista(1, 0);  Check(g4->Ady(1, 0) == 1 && g4->ContarAristas() == 2, "arcos en ambos sentidos");

	// Ronda 2: casos de la Lectura 5.1
	Console::WriteLine("Ronda 2: ejemplo y practica de la Lectura 5.1");
	Grafo^ lect = Arcos(5, gcnew array<int>{ 1, 2, 2, 3, 2, 4, 2, 5, 3, 5, 5, 3, 4, 1 });
	Esperar(lect, gcnew array<String^>{ "{1, 2, 4}", "{3, 5}" }, "Lectura 5.1, ejemplo 1: V1 = {1;2;4}, V2 = {3;5}");
	Resultado^ rl = Algoritmos::Matricial(lect);
	Check(rl->orden[0] == 0 && rl->orden[1] == 1 && rl->orden[2] == 3 && rl->orden[3] == 2 && rl->orden[4] == 4,
		"Lectura 5.1, ejemplo 1: orden de filas 1, 2, 4, 3, 5");
	// Practica 1 (A..E): A>B, B>B, B>C, C>D, D>B, D>E
	Grafo^ prac = Arcos(5, gcnew array<int>{ 1, 2, 2, 2, 2, 3, 3, 4, 4, 2, 4, 5 });
	Esperar(prac, gcnew array<String^>{ "{1}", "{2, 3, 4}", "{5}" }, "Lectura 5.1, practica 1: {A}, {B, C, D}, {E}");

	// Ronda 3: todos los grafos dirigidos de 4 nodos con lazos (2^16)
	Console::WriteLine("Ronda 3: los 65536 grafos dirigidos de 4 nodos (con lazos)");
	for (int mask = 0; mask < (1 << 16); mask++) {
		Grafo^ g = gcnew Grafo(4, true);
		for (int b = 0; b < 16; b++) if ((mask >> b) & 1) g->PonerArista(b / 4, b % 4);
		Verificar(g);
	}

	// Rondas 4-11: grafos dirigidos aleatorios de 5 a 12 nodos, densidades 0 a 0,9, con lazos
	Random^ rnd = gcnew Random(2026);
	for (int n = 5; n <= 12; n++) {
		Console::WriteLine("Ronda " + (n - 1).ToString() + ": 3000 grafos dirigidos aleatorios de " + n.ToString() + " nodos");
		for (int t = 0; t < 3000; t++) {
			Grafo^ g = gcnew Grafo(n, true);
			g->Aleatorio(rnd, (t % 10) / 10.0, 0.15);
			Verificar(g);
		}
	}

	// Ronda 12: casos limite
	Console::WriteLine("Ronda 12: casos limite (vacio, ciclo, camino, completo, estrella, todos con lazo)");
	Grafo^ vacio = gcnew Grafo(12, true);
	Check(Algoritmos::Matricial(vacio)->componentes->Count == 12, "grafo vacio: 12 bloques de 1x1");
	Grafo^ ciclo = gcnew Grafo(12, true);
	for (int i = 0; i < 12; i++) ciclo->PonerArista(i, (i + 1) % 12);
	Check(Algoritmos::Matricial(ciclo)->componentes->Count == 1, "ciclo dirigido de 12: un solo bloque");
	Grafo^ camino = gcnew Grafo(12, true);   // 1 > 2 > ... > 12: nadie puede volver
	for (int i = 0; i < 11; i++) camino->PonerArista(i, i + 1);
	Resultado^ rc = Algoritmos::Matricial(camino);
	Check(rc->componentes->Count == 12 && rc->orden[0] == 0 && rc->orden[11] == 11, "camino dirigido: 12 bloques en orden 1..12");
	Grafo^ completo = gcnew Grafo(12, true);
	for (int i = 0; i < 12; i++) for (int j = 0; j < 12; j++) if (i != j) completo->PonerArista(i, j);
	Check(Algoritmos::Matricial(completo)->componentes->Count == 1, "completo: un bloque de 12x12");
	Grafo^ estrella = gcnew Grafo(8, true);
	for (int i = 0; i < 7; i++) { estrella->PonerArista(7, i); estrella->PonerArista(i, 7); }
	Check(Algoritmos::Dfs(estrella)->componentes->Count == 1, "estrella de ida y vuelta");
	Grafo^ lazos = gcnew Grafo(6, true);
	for (int i = 0; i < 6; i++) lazos->PonerArista(i, i);
	Check(Algoritmos::Matricial(lazos)->componentes->Count == 6, "solo lazos: 6 bloques");
	for each (Grafo ^ g in gcnew array<Grafo^>{ vacio, ciclo, camino, completo, estrella, lazos }) Verificar(g);

	// Ronda 13: ejemplo del informe
	Console::WriteLine("Ronda 13: ejemplo del informe");
	Grafo^ ej = Arcos(7, gcnew array<int>{ 1, 2, 6, 2, 4, 6, 3, 5, 5, 3, 7, 7 });
	Esperar(ej, gcnew array<String^>{ "{4}", "{1}", "{6}", "{3, 5}", "{2}", "{7}" }, "ejemplo del informe: bloques {4} {1} {6} {3,5} {2} {7}");
	Resultado^ r = Algoritmos::Matricial(ej);
	Check(r->pasos->Count == 19, "ejemplo del informe: 19 pasos");

	if (args->Length > 0 && args[0] == "--traza") {
		for (int k = 0; k < r->pasos->Count; k++)
			Console::WriteLine((k + 1).ToString() + ". " + r->pasos[k]->titulo + Environment::NewLine + "    " + r->pasos[k]->texto);
		Resultado^ d = Algoritmos::Dfs(ej);
		Console::WriteLine("--- DFS ---");
		for (int k = 0; k < d->pasos->Count; k++) Console::WriteLine((k + 1).ToString() + ". " + d->pasos[k]->titulo);
	}
	if (args->Length > 0 && args[0] == "--estadisticas") {
		Console::WriteLine("n;componentes_prom;pasos_prom;pasos_min;pasos_max");
		Random^ re = gcnew Random(7);
		for (int n = 4; n <= 12; n++) {
			double sc = 0, sp = 0; int mn = 1000, mx = 0; const int T = 2000;
			for (int t = 0; t < T; t++) {
				Grafo^ g = gcnew Grafo(n, true);
				g->Aleatorio(re, Math::Min(0.45, 1.6 / n), 0.12);   // mismo generador que el boton Aleatorio
				Resultado^ a1 = Algoritmos::Matricial(g);
				sc += a1->componentes->Count; sp += a1->pasos->Count;
				mn = Math::Min(mn, a1->pasos->Count); mx = Math::Max(mx, a1->pasos->Count);
			}
			Console::WriteLine(n.ToString() + ";" + (sc / T).ToString("0.00") + ";" + (sp / T).ToString("0.0") + ";" + mn.ToString() + ";" + mx.ToString());
		}
	}

	Console::WriteLine();
	Console::WriteLine("Comprobaciones: " + pruebas.ToString() + "   Fallos: " + fallos.ToString());
	return fallos == 0 ? 0 : 1;
}
