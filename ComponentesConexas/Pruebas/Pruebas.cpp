// ============================================================
//  Pruebas.cpp  -  Pruebas automaticas de los algoritmos.
//  Compilar desde "Developer Command Prompt for VS 2022":
//     cl /clr /EHa /utf-8 /nologo /I..\ComponentesConexas Pruebas.cpp
//  y ejecutar Pruebas.exe. Devuelve 0 si todas las pruebas pasan.
// ============================================================
#include "Algoritmo.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace ComponentesConexas;

static int fallos = 0;
static int pruebas = 0;

static void Check(bool cond, String^ msg) {
	pruebas++;
	if (!cond) { fallos++; if (fallos <= 20) Console::WriteLine("  FALLO: " + msg); }
}

static Grafo^ DesdeMascara(int n, long long mask) {
	Grafo^ g = gcnew Grafo(n);
	int bit = 0;
	for (int i = 0; i < n; i++)
		for (int j = i + 1; j < n; j++, bit++)
			if ((mask >> bit) & 1) g->PonerArista(i, j);
	return g;
}

static String^ Desc(Grafo^ g) {
	String^ s = "n=" + g->N.ToString() + (g->Dirigido ? " arcos:" : " aristas:");
	for (int i = 0; i < g->N; i++)
		for (int j = g->Dirigido ? 0 : i; j < g->N; j++)
			if (g->Ady(i, j)) s += " " + (i + 1).ToString() + (g->Dirigido ? ">" : "-") + (j + 1).ToString();
	return s;
}

// Fuertemente conexas por fuerza bruta: alcanzabilidad con DFS desde cada nodo.
static List<List<int>^>^ FuertesBruta(Grafo^ g) {
	int n = g->N;
	array<bool, 2>^ alc = gcnew array<bool, 2>(n, n);
	for (int s0 = 0; s0 < n; s0++) {
		Stack<int>^ st = gcnew Stack<int>();
		st->Push(s0); alc[s0, s0] = true;
		while (st->Count > 0) {
			int u = st->Pop();
			for (int v = 0; v < n; v++) if (g->Ady(u, v) && !alc[s0, v]) { alc[s0, v] = true; st->Push(v); }
		}
	}
	List<List<int>^>^ res = gcnew List<List<int>^>();
	array<bool>^ usado = gcnew array<bool>(n);
	for (int i = 0; i < n; i++) {
		if (usado[i]) continue;
		List<int>^ grupo = gcnew List<int>();
		for (int j = 0; j < n; j++) if (!usado[j] && alc[i, j] && alc[j, i]) { grupo->Add(j); usado[j] = true; }
		res->Add(grupo);
	}
	return res;
}

// Verifica resultados e invariantes de los pasos para un grafo.
static void Verificar(Grafo^ g) {
	List<List<int>^>^ ref = Algoritmos::Referencia(g);
	Resultado^ m = Algoritmos::Matricial(g);
	Resultado^ d = Algoritmos::Dfs(g);
	Check(Algoritmos::Iguales(ref, m->componentes), "matricial distinto de referencia: " + Desc(g));
	Check(Algoritmos::Iguales(ref, d->componentes), "DFS distinto de referencia: " + Desc(g));

	int n = g->N;
	for each (Resultado ^ r in gcnew array<Resultado^>{ m, d }) {
		Check(r->pasos->Count >= 2, "pocos pasos");
		Check(r->pasos[r->pasos->Count - 1]->tipo == TipoPaso::Fin, "el ultimo paso no es Fin");
		Check(r->pasos[0]->tipo == TipoPaso::Inicio, "el primer paso no es Inicio");
		// Todos los nodos quedan asignados y cada uno en exactamente una componente.
		array<int>^ cuenta = gcnew array<int>(n);
		for each (List<int> ^ c in r->componentes) for each (int v in c) cuenta[v]++;
		for (int v = 0; v < n; v++) Check(cuenta[v] == 1 && r->comp[v] >= 0, "nodo sin componente o repetido");
		// Una componente asignada nunca cambia de valor en pasos posteriores.
		array<int>^ visto = gcnew array<int>(n);
		for (int v = 0; v < n; v++) visto[v] = -1;
		for each (Paso ^ p in r->pasos) {
			for (int v = 0; v < n; v++) {
				if (visto[v] != -1) Check(p->comp[v] == visto[v], "comp cambia despues de asignarse");
				if (p->comp[v] != -1) visto[v] = p->comp[v];
			}
			Check(p->titulo != nullptr && p->texto != nullptr && p->texto->Length > 0, "paso sin texto");
		}
	}

	// Matriz final del metodo matricial: diagonal en 1, simetrica y
	// dos filas son iguales si y solo si los nodos estan en la misma componente.
	array<int, 2>^ R = m->pasos[m->pasos->Count - 1]->matriz;
	for (int i = 0; i < n; i++) {
		Check(R[i, i] == 1, "diagonal sin 1");
		for (int j = 0; j < n; j++) {
			Check(R[i, j] == R[j, i], "matriz final no simetrica");
			bool misma = m->comp[i] == m->comp[j];
			Check((R[i, j] == 1) == misma, "R[i,j] no coincide con la componente");
		}
	}
	// El paso de la diagonal: M = S + I, con S = A (no dirigido) o S = A OR A^T (dirigido).
	int iDiag = g->Dirigido ? 2 : 1;
	Paso^ pd = m->pasos[iDiag];
	Check(pd->tipo == TipoPaso::Diagonal, "el paso de la diagonal no esta donde corresponde");
	if (g->Dirigido) Check(m->pasos[1]->tipo == TipoPaso::Simetrizar && d->pasos[1]->tipo == TipoPaso::Simetrizar, "falta el paso de simetrizacion");
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			Check(pd->matriz[i, j] == (i == j ? 1 : (g->Conectados(i, j) ? 1 : 0)), "M = S + I incorrecta: " + Desc(g));
	// El paso 1 muestra la matriz original sin modificar.
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++) Check(m->pasos[0]->matriz[i, j] == g->Ady(i, j), "el paso 1 no muestra A");
	// Cantidad de pasos: matricial 2n + 3 y DFS 3n + 2 (un paso mas si hay que simetrizar).
	int extra = g->Dirigido ? 1 : 0;
	Check(m->pasos->Count == 2 * n + 3 + extra, "cantidad de pasos matricial");
	Check(d->pasos->Count == 3 * n + 2 + extra, "cantidad de pasos DFS");
	// Fuertemente conexas: solo en grafos dirigidos y comparadas con fuerza bruta.
	if (g->Dirigido) {
		Check(m->fuertes != nullptr && Algoritmos::Iguales(m->fuertes, FuertesBruta(g)), "fuertemente conexas incorrectas: " + Desc(g));
		Check(m->fuertes->Count >= m->componentes->Count, "hay menos fuertes que conexas");
	}
	else Check(m->fuertes == nullptr && d->fuertes == nullptr, "fuertes en grafo no dirigido");
}

int main(array<String^>^ args) {
	// Ronda 1: validacion de entradas del modelo
	Console::WriteLine("Ronda 1: validacion de entradas del grafo");
	for each (int malo in gcnew array<int>{ -1, 0, 3, 13, 100 }) {
		bool lanzo = false;
		try { gcnew Grafo(malo); }
		catch (ArgumentOutOfRangeException^) { lanzo = true; }
		Check(lanzo, "Grafo(" + malo.ToString() + ") deberia lanzar excepcion");
	}
	Grafo^ g4 = gcnew Grafo(4);
	g4->AlternarArista(2, 2);  Check(g4->Ady(2, 2) == 1 && g4->ContarLazos() == 1, "no se pudo crear un lazo");
	g4->AlternarArista(2, 2);  Check(g4->Ady(2, 2) == 0, "no se pudo quitar el lazo");
	g4->AlternarArista(-1, 2); g4->AlternarArista(0, 9);
	Check(g4->ContarAristas() == 0, "arista fuera de rango aceptada");
	// Grafo dirigido: el arco 1 -> 2 no crea 2 -> 1; al pasar a no dirigido se simetriza.
	Grafo^ gd = gcnew Grafo(4, true);
	gd->AlternarArista(0, 1);  Check(gd->Ady(0, 1) == 1 && gd->Ady(1, 0) == 0, "el arco dirigido se duplico");
	gd->AlternarArista(1, 0);  Check(gd->Ady(1, 0) == 1 && gd->ContarAristas() == 2, "arcos en ambos sentidos");
	gd->AlternarArista(1, 0);
	gd->AlternarArista(3, 2);
	gd->Dirigido = false;      Check(gd->Ady(0, 1) == 1 && gd->Ady(1, 0) == 1 && gd->Ady(2, 3) == 1, "no se simetrizo al cambiar de tipo");
	Check(gd->ContarAristas() == 2, "conteo de aristas tras simetrizar");
	g4->AlternarArista(0, 3);  Check(g4->Ady(0, 3) == 1 && g4->Ady(3, 0) == 1, "arista no simetrica");
	g4->AlternarArista(3, 0);  Check(g4->Ady(0, 3) == 0 && g4->Ady(3, 0) == 0, "no se pudo quitar la arista");

	// Rondas 2-4: todos los grafos posibles de 4, 5 y 6 nodos
	for (int n = 4; n <= 6; n++) {
		long long total = 1LL << (n * (n - 1) / 2);
		Console::WriteLine("Ronda " + (n - 2).ToString() + ": los " + total.ToString() + " grafos de " + n.ToString() + " nodos");
		for (long long mask = 0; mask < total; mask++) Verificar(DesdeMascara(n, mask));
	}

	// Rondas 5-10: grafos aleatorios de 7 a 12 nodos con distintas densidades
	Random^ rnd = gcnew Random(2026);
	for (int n = 7; n <= 12; n++) {
		Console::WriteLine("Ronda " + (n - 2).ToString() + ": 3000 grafos aleatorios de " + n.ToString() + " nodos");
		for (int t = 0; t < 3000; t++) {
			Grafo^ g = gcnew Grafo(n);
			g->Aleatorio(rnd, (t % 10) / 10.0);
			Verificar(g);
		}
	}

	// Ronda 10b: todos los grafos dirigidos de 4 nodos con lazos (2^16 = 65 536)
	Console::WriteLine("Ronda 10b: los 65536 grafos dirigidos de 4 nodos (con lazos)");
	for (int mask = 0; mask < (1 << 16); mask++) {
		Grafo^ g = gcnew Grafo(4, true);
		for (int b = 0; b < 16; b++) if ((mask >> b) & 1) g->PonerArista(b / 4, b % 4);
		Verificar(g);
	}
	// Ronda 10c: grafos dirigidos aleatorios de 5 a 12 nodos, con lazos
	Console::WriteLine("Ronda 10c: 2000 grafos dirigidos aleatorios por cada n de 5 a 12");
	for (int n = 5; n <= 12; n++)
		for (int t = 0; t < 2000; t++) {
			Grafo^ g = gcnew Grafo(n, true);
			g->Aleatorio(rnd, (t % 10) / 10.0, 0.15);
			Verificar(g);
		}
	// Ronda 10d: grafos no dirigidos con lazos
	Console::WriteLine("Ronda 10d: 2000 grafos no dirigidos con lazos por cada n de 4 a 12");
	for (int n = 4; n <= 12; n++)
		for (int t = 0; t < 2000; t++) {
			Grafo^ g = gcnew Grafo(n);
			g->Aleatorio(rnd, (t % 10) / 10.0, 0.3);
			Verificar(g);
		}
	// Caso del grafo dirigido de demostracion: ciclo 1 -> 2 -> 3 -> 1, 2 -> 5, lazos en 1 y 4.
	{
		Grafo^ g = gcnew Grafo(5, true);
		g->PonerArista(0, 0); g->PonerArista(0, 1); g->PonerArista(0, 2); g->PonerArista(2, 0);
		g->PonerArista(1, 2); g->PonerArista(1, 4); g->PonerArista(3, 3);
		Resultado^ r = Algoritmos::Matricial(g);
		Check(r->componentes->Count == 2 && Algoritmos::Conjunto(r->componentes[0]) == "{1, 2, 3, 5}", "demo dirigida: conexas");
		Check(r->fuertes->Count == 3 && Algoritmos::Conjunto(r->fuertes[0]) == "{1, 2, 3}", "demo dirigida: fuertes");
	}

	// Ronda 11: casos limite
	Console::WriteLine("Ronda 11: casos limite (vacio, completo, camino, estrella, ultimo nodo aislado)");
	Grafo^ vacio = gcnew Grafo(12);
	Check(Algoritmos::Matricial(vacio)->componentes->Count == 12, "grafo vacio de 12 nodos");
	Grafo^ completo = gcnew Grafo(12);
	for (int i = 0; i < 12; i++) for (int j = i + 1; j < 12; j++) completo->PonerArista(i, j);
	Check(Algoritmos::Matricial(completo)->componentes->Count == 1, "grafo completo de 12 nodos");
	Grafo^ camino = gcnew Grafo(12);   // 12-11-...-1: obliga a muchas sumas de filas
	for (int i = 0; i < 11; i++) camino->PonerArista(i, i + 1);
	Check(Algoritmos::Matricial(camino)->componentes->Count == 1, "grafo camino");
	Grafo^ estrella = gcnew Grafo(8);
	for (int i = 0; i < 7; i++) estrella->PonerArista(7, i);
	Check(Algoritmos::Dfs(estrella)->componentes->Count == 1, "grafo estrella con centro en el ultimo nodo");
	Grafo^ aislado = gcnew Grafo(5);
	aislado->PonerArista(0, 1); aislado->PonerArista(1, 2); aislado->PonerArista(2, 3);
	Check(Algoritmos::Matricial(aislado)->componentes->Count == 2, "ultimo nodo aislado");
	for each (Grafo ^ g in gcnew array<Grafo^>{ vacio, completo, camino, estrella, aislado }) Verificar(g);

	// Ronda 12: ejemplo del informe (resuelto a mano)
	Console::WriteLine("Ronda 12: ejemplo del informe");
	Grafo^ ej = gcnew Grafo(7, true);   // arcos 1>2, 6>2, 4>6, 3>5, 5>3 y lazo en 7
	ej->PonerArista(0, 1); ej->PonerArista(5, 1); ej->PonerArista(3, 5); ej->PonerArista(2, 4); ej->PonerArista(4, 2); ej->PonerArista(6, 6);
	Resultado^ r = Algoritmos::Matricial(ej);
	Check(r->componentes->Count == 3, "ejemplo: 3 componentes");
	Check(Algoritmos::Conjunto(r->componentes[0]) == "{1, 2, 4, 6}", "ejemplo: C1");
	Check(Algoritmos::Conjunto(r->componentes[1]) == "{3, 5}", "ejemplo: C2");
	Check(Algoritmos::Conjunto(r->componentes[2]) == "{7}", "ejemplo: C3");
	Check(r->pasos->Count == 18 && r->pasos[1]->tipo == TipoPaso::Simetrizar, "ejemplo: 18 pasos con simetrizacion");
	Check(r->fuertes->Count == 6 && Algoritmos::Conjunto(r->fuertes[2]) == "{3, 5}", "ejemplo: fuertemente conexas");
	Verificar(ej);
	if (args->Length > 0 && args[0] == "--estadisticas") {
		Console::WriteLine("n;componentes_prom;fuertes_prom;pasos_matricial_prom;pasos_dfs_prom;pasos_matricial_max;pasos_dfs_max");
		Random^ re = gcnew Random(7);
		for (int n = 4; n <= 12; n++) {
			double sc = 0, sf = 0, sm = 0, sd = 0; int mm = 0, md = 0; const int T = 2000;
			for (int t = 0; t < T; t++) {
				Grafo^ g = gcnew Grafo(n, true);
				g->Aleatorio(re, Math::Min(0.45, 1.6 / n), 0.12);   // mismo generador que el boton Aleatorio
				Resultado^ a1 = Algoritmos::Matricial(g); Resultado^ a2 = Algoritmos::Dfs(g);
				sc += a1->componentes->Count; sf += a1->fuertes->Count; sm += a1->pasos->Count; sd += a2->pasos->Count;
				mm = Math::Max(mm, a1->pasos->Count); md = Math::Max(md, a2->pasos->Count);
			}
			Console::WriteLine(n.ToString() + ";" + (sc / T).ToString("0.00") + ";" + (sf / T).ToString("0.00") + ";" + (sm / T).ToString("0.0") + ";" + (sd / T).ToString("0.0") + ";" + mm.ToString() + ";" + md.ToString());
		}
	}
	if (args->Length > 0 && args[0] == "--traza") {
		for (int k = 0; k < r->pasos->Count; k++)
			Console::WriteLine((k + 1).ToString() + ". " + r->pasos[k]->titulo + Environment::NewLine + "    " + r->pasos[k]->texto);
		Resultado^ d = Algoritmos::Dfs(ej);
		Console::WriteLine("--- DFS ---");
		for (int k = 0; k < d->pasos->Count; k++) Console::WriteLine((k + 1).ToString() + ". " + d->pasos[k]->titulo);
	}

	Console::WriteLine();
	Console::WriteLine("Comprobaciones: " + pruebas.ToString() + "   Fallos: " + fallos.ToString());
	return fallos == 0 ? 0 : 1;
}
