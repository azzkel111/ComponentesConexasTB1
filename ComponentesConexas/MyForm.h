#pragma once
// ============================================================
//  MyForm.h
//  Interfaz principal. Responsabilidades:
//   - construir el grafo (manual, aleatorio o ejemplo del informe)
//   - lanzar el algoritmo elegido y recorrer sus pasos
//     (Anterior / Siguiente / Reproducir / Ver resultado)
//   - mantener el estado de la interfaz coherente (ActualizarUI)
//  El calculo vive en Algoritmo.h y el dibujo en Render.h.
// ============================================================

#include "Grafo.h"
#include "Algoritmo.h"
#include "Render.h"

namespace ComponentesConexas {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections::Generic;
	using namespace System::Windows::Forms;
	using namespace System::Drawing;

	// Estados de la aplicacion
	public enum class Modo { SinGrafo, Edicion, Ejecucion };

	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			rnd = gcnew Random();
			modo = Modo::SinGrafo;
			seleccionado = hoverNodo = arrastrado = -1;
			hoverI = hoverJ = -1;
			idx = 0;
			ActualizarUI();
		}

		// Genera las capturas usadas en el informe y la presentacion (opcion --capturas <carpeta>).
		void ProgramarCapturas(String^ carpeta) { carpetaCapturas = carpeta; }

		// Prueba de estres (opcion --estres <acciones>): ejecuta acciones aleatorias de la
		// interfaz y verifica invariantes. El codigo de salida queda en CodigoSalida.
		void ProgramarEstres(int acciones) { accionesEstres = acciones; }
		property int CodigoSalida { int get() { return codigoSalida; } }

	protected:
		~MyForm()
		{
			if (components) delete components;
		}

		// ---------------- Controles ----------------
	private:
		System::ComponentModel::Container^ components;
		Panel^ raiz;
		Panel^ barraTitulo;
		FlowLayoutPanel^ barraHerramientas;
		NumericUpDown^ nudN;
		Button^ btnCrear;
		Button^ btnManual;
		Button^ btnAleatorio;
		Button^ btnEjemplo;
		Button^ btnReorganizar;
		ComboBox^ cboMetodo;
		Button^ btnIniciar;
		SplitContainer^ division;
		Lienzo^ lienzoGrafo;
		Lienzo^ lienzoMatriz;
		Lienzo^ lienzoResultado;
		ListBox^ lstPasos;
		Panel^ barraPasos;
		Label^ lblPasoNum;
		Label^ lblPasoTitulo;
		Label^ lblPasoTexto;
		Button^ btnReiniciar;
		Button^ btnAnterior;
		Button^ btnSiguiente;
		Button^ btnReproducir;
		Button^ btnFinal;
		System::Windows::Forms::Timer^ temporizador;

		// ---------------- Estado ----------------
		Modo modo;
		Grafo^ grafo;
		array<PointF>^ pos;          // posiciones normalizadas (0..1) de los nodos
		Resultado^ res;              // pasos del metodo elegido
		bool coincide;               // el otro metodo da el mismo resultado
		String^ nombreOtro;
		int idx;                     // paso mostrado
		int seleccionado;            // primer nodo elegido para crear una arista
		int hoverNodo, arrastrado;
		Point inicioArrastre;
		bool seMovio;
		int hoverI, hoverJ;          // celda bajo el mouse en la matriz
		bool sincronizandoLista;
		Random^ rnd;
		String^ carpetaCapturas;
		int accionesEstres;
		int codigoSalida;

#pragma region Construccion de la interfaz
		Button^ Boton(String^ texto, int ancho, bool primario) {
			Button^ b = gcnew Button();
			b->Text = texto;
			b->Size = Drawing::Size(ancho, 34);
			b->Margin = System::Windows::Forms::Padding(3, 9, 3, 9);
			b->FlatStyle = FlatStyle::Flat;
			b->Cursor = Cursors::Hand;
			b->Font = primario ? Tema::Negrita : Tema::Normal;
			b->BackColor = primario ? Tema::Acento : Color::White;
			b->ForeColor = primario ? Color::White : Tema::Texto;
			b->FlatAppearance->BorderColor = primario ? Tema::Acento : Color::FromArgb(203, 213, 225);
			b->FlatAppearance->MouseOverBackColor = primario ? Color::FromArgb(29, 78, 216) : Tema::Fondo;
			b->UseVisualStyleBackColor = false;
			return b;
		}

		Label^ Texto(String^ t, Drawing::Font^ f, Color c) {
			Label^ l = gcnew Label();
			l->Text = t;
			l->Font = f;
			l->ForeColor = c;
			l->AutoSize = true;
			l->Margin = System::Windows::Forms::Padding(8, 17, 2, 0);
			return l;
		}

		Label^ Separador() {
			Label^ l = gcnew Label();
			l->AutoSize = false;
			l->Size = Drawing::Size(1, 30);
			l->BackColor = Tema::Borde;
			l->Margin = System::Windows::Forms::Padding(10, 11, 10, 11);
			return l;
		}

		Panel^ Tarjeta(Control^ contenido) {
			Panel^ p = gcnew Panel();
			p->Dock = DockStyle::Fill;
			p->Padding = System::Windows::Forms::Padding(1);
			p->BackColor = Tema::Borde;              // el padding de 1px actua como borde
			p->Margin = System::Windows::Forms::Padding(6);
			contenido->Dock = DockStyle::Fill;
			p->Controls->Add(contenido);
			return p;
		}

		void InitializeComponent(void)
		{
			this->components = gcnew System::ComponentModel::Container();
			this->SuspendLayout();

			// ---- Barra de titulo ----
			barraTitulo = gcnew Panel();
			barraTitulo->Dock = DockStyle::Top;
			barraTitulo->Height = 54;
			barraTitulo->BackColor = Tema::Cabecera;
			Label^ titulo = gcnew Label();
			titulo->Text = "Componentes conexas de un grafo";
			titulo->Font = gcnew Drawing::Font("Segoe UI Semibold", 15.0f);
			titulo->ForeColor = Color::White;
			titulo->AutoSize = true;
			titulo->Location = Point(16, 11);
			Label^ subtitulo = gcnew Label();
			subtitulo->Text = "Matemática Computacional · Grupo 2 · Grafo dirigido · Lectura 5.1: matriz de caminos y bloques diagonales";
			subtitulo->Font = Tema::Normal;
			subtitulo->ForeColor = Color::FromArgb(148, 163, 184);
			subtitulo->AutoSize = true;
			subtitulo->Location = Point(370, 19);
			barraTitulo->Controls->Add(titulo);
			barraTitulo->Controls->Add(subtitulo);

			// ---- Barra de herramientas ----
			barraHerramientas = gcnew FlowLayoutPanel();
			barraHerramientas->Dock = DockStyle::Top;
			barraHerramientas->Height = 54;
			barraHerramientas->BackColor = Color::White;
			barraHerramientas->Padding = System::Windows::Forms::Padding(8, 0, 8, 0);
			barraHerramientas->WrapContents = false;

			nudN = gcnew NumericUpDown();
			nudN->Minimum = Grafo::MIN_NODOS;
			nudN->Maximum = Grafo::MAX_NODOS;
			nudN->Value = 7;
			nudN->Width = 52;
			nudN->Font = gcnew Drawing::Font("Segoe UI", 11.0f);
			nudN->Margin = System::Windows::Forms::Padding(4, 12, 4, 0);
			nudN->TextAlign = HorizontalAlignment::Center;

			btnCrear = Boton("Crear grafo", 100, true);
			btnManual = Boton("Manual", 78, false);
			btnAleatorio = Boton("Aleatorio", 84, false);
			btnEjemplo = Boton("Ejemplo", 78, false);
			btnReorganizar = Boton("Reorganizar", 96, false);
			Label^ tipo = Texto("Grafo dirigido \u2192", Tema::Negrita, Tema::Acento);
			cboMetodo = gcnew ComboBox();
			cboMetodo->DropDownStyle = ComboBoxStyle::DropDownList;
			cboMetodo->Items->Add("Matriz de caminos: Warshall");
			cboMetodo->Items->Add("Matriz de caminos: DFS");
			cboMetodo->SelectedIndex = 0;
			cboMetodo->Width = 200;
			cboMetodo->Font = Tema::Normal;
			cboMetodo->Margin = System::Windows::Forms::Padding(4, 14, 4, 0);
			btnIniciar = Boton("Iniciar algoritmo", 140, true);

			barraHerramientas->Controls->Add(Texto("Nodos (4–12):", Tema::Negrita, Tema::Texto));
			barraHerramientas->Controls->Add(nudN);
			barraHerramientas->Controls->Add(btnCrear);
			barraHerramientas->Controls->Add(Separador());
			barraHerramientas->Controls->Add(btnManual);
			barraHerramientas->Controls->Add(btnAleatorio);
			barraHerramientas->Controls->Add(btnEjemplo);
			barraHerramientas->Controls->Add(btnReorganizar);
			barraHerramientas->Controls->Add(tipo);
			barraHerramientas->Controls->Add(Separador());
			barraHerramientas->Controls->Add(Texto("Método:", Tema::Negrita, Tema::Texto));
			barraHerramientas->Controls->Add(cboMetodo);
			barraHerramientas->Controls->Add(btnIniciar);

			// ---- Barra inferior: explicacion del paso y navegacion ----
			barraPasos = gcnew Panel();
			barraPasos->Dock = DockStyle::Bottom;
			barraPasos->Height = 104;
			barraPasos->BackColor = Color::White;
			barraPasos->Padding = System::Windows::Forms::Padding(12, 10, 12, 10);

			lblPasoNum = gcnew Label();
			lblPasoNum->Dock = DockStyle::Left;
			lblPasoNum->Width = 104;
			lblPasoNum->Font = gcnew Drawing::Font("Segoe UI Semibold", 13.0f);
			lblPasoNum->ForeColor = Color::White;
			lblPasoNum->BackColor = Tema::Cabecera;
			lblPasoNum->TextAlign = ContentAlignment::MiddleCenter;

			FlowLayoutPanel^ nav = gcnew FlowLayoutPanel();
			nav->Dock = DockStyle::Right;
			nav->Width = 600;
			nav->WrapContents = false;
			nav->Padding = System::Windows::Forms::Padding(0, 14, 0, 0);
			btnReiniciar = Boton("⏮ Inicio", 92, false);
			btnAnterior = Boton("◀ Anterior", 104, false);
			btnSiguiente = Boton("Siguiente ▶", 124, true);
			btnReproducir = Boton("▶ Reproducir", 116, false);
			btnFinal = Boton("Ver resultado ⏭", 134, false);
			nav->Controls->Add(btnReiniciar);
			nav->Controls->Add(btnAnterior);
			nav->Controls->Add(btnSiguiente);
			nav->Controls->Add(btnReproducir);
			nav->Controls->Add(btnFinal);

			Panel^ textos = gcnew Panel();
			textos->Dock = DockStyle::Fill;
			textos->Padding = System::Windows::Forms::Padding(14, 0, 10, 0);
			lblPasoTitulo = gcnew Label();
			lblPasoTitulo->Dock = DockStyle::Top;
			lblPasoTitulo->Height = 28;
			lblPasoTitulo->Font = gcnew Drawing::Font("Segoe UI Semibold", 12.0f);
			lblPasoTitulo->ForeColor = Tema::Texto;
			lblPasoTitulo->AutoEllipsis = true;
			lblPasoTexto = gcnew Label();
			lblPasoTexto->Dock = DockStyle::Fill;
			lblPasoTexto->Font = Tema::Normal;
			lblPasoTexto->ForeColor = Tema::TextoSuave;
			lblPasoTexto->AutoEllipsis = true;
			textos->Controls->Add(lblPasoTexto);
			textos->Controls->Add(lblPasoTitulo);

			barraPasos->Controls->Add(textos);
			barraPasos->Controls->Add(nav);
			barraPasos->Controls->Add(lblPasoNum);

			// ---- Zona central: grafo + resultado | matriz + registro de pasos ----
			lienzoGrafo = gcnew Lienzo();
			lienzoMatriz = gcnew Lienzo();
			lienzoResultado = gcnew Lienzo();

			lstPasos = gcnew ListBox();
			lstPasos->BorderStyle = BorderStyle::None;
			lstPasos->DrawMode = DrawMode::OwnerDrawFixed;
			lstPasos->ItemHeight = 24;
			lstPasos->Font = Tema::Normal;
			lstPasos->IntegralHeight = false;

			TableLayoutPanel^ izq = gcnew TableLayoutPanel();
			izq->Dock = DockStyle::Fill;
			izq->ColumnCount = 1;
			izq->RowCount = 2;
			izq->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));
			izq->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 124));
			izq->Controls->Add(Tarjeta(lienzoGrafo), 0, 0);
			izq->Controls->Add(Tarjeta(lienzoResultado), 0, 1);

			Panel^ panelPasos = gcnew Panel();
			panelPasos->BackColor = Color::White;
			Label^ tituloPasos = gcnew Label();
			tituloPasos->Text = "  Registro de pasos  (clic en un paso para volver a él)";
			tituloPasos->Dock = DockStyle::Top;
			tituloPasos->Height = 30;
			tituloPasos->Font = Tema::Negrita;
			tituloPasos->ForeColor = Tema::Texto;
			tituloPasos->TextAlign = ContentAlignment::MiddleLeft;
			lstPasos->Dock = DockStyle::Fill;
			panelPasos->Controls->Add(lstPasos);
			panelPasos->Controls->Add(tituloPasos);

			TableLayoutPanel^ der = gcnew TableLayoutPanel();
			der->Dock = DockStyle::Fill;
			der->ColumnCount = 1;
			der->RowCount = 2;
			der->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 70));
			der->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 30));
			der->Controls->Add(Tarjeta(lienzoMatriz), 0, 0);
			der->Controls->Add(Tarjeta(panelPasos), 0, 1);

			division = gcnew SplitContainer();
			division->Dock = DockStyle::Fill;
			division->Orientation = Orientation::Vertical;
			division->SplitterWidth = 6;
			division->TabStop = false;
			division->BackColor = Tema::Fondo;
			division->Panel1->Controls->Add(izq);
			division->Panel2->Controls->Add(der);

			raiz = gcnew Panel();
			raiz->Dock = DockStyle::Fill;
			raiz->BackColor = Tema::Fondo;
			Panel^ centro = gcnew Panel();
			centro->Dock = DockStyle::Fill;
			centro->Padding = System::Windows::Forms::Padding(6);
			centro->Controls->Add(division);
			raiz->Controls->Add(centro);
			raiz->Controls->Add(barraPasos);
			raiz->Controls->Add(barraHerramientas);
			raiz->Controls->Add(barraTitulo);

			// ---- Temporizador de reproduccion automatica ----
			temporizador = gcnew System::Windows::Forms::Timer(this->components);
			temporizador->Interval = 1100;

			// ---- Formulario ----
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::None;
			this->ClientSize = Drawing::Size(1366, 800);
			this->MinimumSize = Drawing::Size(1240, 780);
			this->Font = Tema::Normal;
			this->BackColor = Tema::Fondo;
			this->StartPosition = FormStartPosition::CenterScreen;
			this->WindowState = FormWindowState::Maximized;
			this->Text = "Componentes conexas de un grafo";
			this->KeyPreview = true;
			this->Controls->Add(raiz);

			// ---- Eventos ----
			btnCrear->Click += gcnew EventHandler(this, &MyForm::btnCrear_Click);
			btnManual->Click += gcnew EventHandler(this, &MyForm::btnManual_Click);
			btnAleatorio->Click += gcnew EventHandler(this, &MyForm::btnAleatorio_Click);
			btnEjemplo->Click += gcnew EventHandler(this, &MyForm::btnEjemplo_Click);
			btnReorganizar->Click += gcnew EventHandler(this, &MyForm::btnReorganizar_Click);
			btnIniciar->Click += gcnew EventHandler(this, &MyForm::btnIniciar_Click);
			cboMetodo->SelectedIndexChanged += gcnew EventHandler(this, &MyForm::cboMetodo_Changed);
			btnReiniciar->Click += gcnew EventHandler(this, &MyForm::btnReiniciar_Click);
			btnAnterior->Click += gcnew EventHandler(this, &MyForm::btnAnterior_Click);
			btnSiguiente->Click += gcnew EventHandler(this, &MyForm::btnSiguiente_Click);
			btnReproducir->Click += gcnew EventHandler(this, &MyForm::btnReproducir_Click);
			btnFinal->Click += gcnew EventHandler(this, &MyForm::btnFinal_Click);
			temporizador->Tick += gcnew EventHandler(this, &MyForm::temporizador_Tick);
			lstPasos->DrawItem += gcnew DrawItemEventHandler(this, &MyForm::lstPasos_DrawItem);
			lstPasos->SelectedIndexChanged += gcnew EventHandler(this, &MyForm::lstPasos_Changed);
			lienzoGrafo->Paint += gcnew PaintEventHandler(this, &MyForm::lienzoGrafo_Paint);
			lienzoGrafo->MouseDown += gcnew MouseEventHandler(this, &MyForm::lienzoGrafo_MouseDown);
			lienzoGrafo->MouseMove += gcnew MouseEventHandler(this, &MyForm::lienzoGrafo_MouseMove);
			lienzoGrafo->MouseUp += gcnew MouseEventHandler(this, &MyForm::lienzoGrafo_MouseUp);
			lienzoMatriz->Paint += gcnew PaintEventHandler(this, &MyForm::lienzoMatriz_Paint);
			lienzoMatriz->MouseMove += gcnew MouseEventHandler(this, &MyForm::lienzoMatriz_MouseMove);
			lienzoMatriz->MouseLeave += gcnew EventHandler(this, &MyForm::lienzoMatriz_MouseLeave);
			lienzoMatriz->MouseClick += gcnew MouseEventHandler(this, &MyForm::lienzoMatriz_MouseClick);
			lienzoResultado->Paint += gcnew PaintEventHandler(this, &MyForm::lienzoResultado_Paint);
			this->Load += gcnew EventHandler(this, &MyForm::MyForm_Load);
			this->Shown += gcnew EventHandler(this, &MyForm::MyForm_Shown);

			this->ResumeLayout(false);
		}
#pragma endregion

		// ============================================================
		//  CONSTRUCCION DEL GRAFO
		// ============================================================
		void NuevoGrafo(int n) {
			CancelarEjecucion();
			grafo = gcnew Grafo(n, true);   // el proyecto trabaja siempre con grafos dirigidos
			nudN->Value = n;          // el selector siempre muestra el n del grafo actual
			pos = Disposicion::Circular(n);
			seleccionado = hoverNodo = arrastrado = -1;
			modo = Modo::Edicion;
		}

		System::Void btnCrear_Click(Object^ sender, EventArgs^ e) {
			NuevoGrafo(Decimal::ToInt32(nudN->Value));
			ActualizarUI();
		}

		System::Void btnManual_Click(Object^ sender, EventArgs^ e) {
			if (grafo == nullptr) return;
			NuevoGrafo(grafo->N);   // conserva n, borra las aristas
			ActualizarUI();
		}

		System::Void btnAleatorio_Click(Object^ sender, EventArgs^ e) {
			if (grafo == nullptr) return;
			GenerarAleatorio();
			ActualizarUI();
		}

		void GenerarAleatorio() {
			int n = grafo->N;
			NuevoGrafo(n);
			// Densidad baja para que aparezcan varias componentes; se evita el grafo vacio.
			double p = Math::Min(0.45, 1.6 / n);
			for (int intento = 0; intento < 20; intento++) {
				grafo->Aleatorio(rnd, p, 0.12);   // ~12 % de los nodos con lazo
				if (grafo->ContarAristas() > 0) break;
			}
			pos = Disposicion::Fuerzas(grafo);
		}

		System::Void btnEjemplo_Click(Object^ sender, EventArgs^ e) {
			CargarEjemplo();
			ActualizarUI();
		}

		// Grafo dirigido resuelto a mano en el informe: 7 nodos,
		// arcos 1 -> 2, 6 -> 2, 4 -> 6, 3 -> 5, 5 -> 3 y un lazo en 7.
		void CargarEjemplo() {
			NuevoGrafo(7);
			grafo->PonerArista(0, 1);
			grafo->PonerArista(5, 1);
			grafo->PonerArista(3, 5);
			grafo->PonerArista(2, 4);
			grafo->PonerArista(4, 2);
			grafo->PonerArista(6, 6);
			pos = gcnew array<PointF>{
				PointF(0.08f, 0.30f), PointF(0.36f, 0.08f), PointF(0.70f, 0.18f), PointF(0.14f, 0.82f),
				PointF(0.95f, 0.44f), PointF(0.42f, 0.56f), PointF(0.80f, 0.88f) };
			cboMetodo->SelectedIndex = 0;
		}

		System::Void btnReorganizar_Click(Object^ sender, EventArgs^ e) {
			if (grafo == nullptr) return;
			pos = Disposicion::Fuerzas(grafo);
			lienzoGrafo->Invalidate();
		}

		void AlternarArista(int i, int j) {
			if (modo != Modo::Edicion || grafo == nullptr) return;
			grafo->AlternarArista(i, j);
			ActualizarUI();
		}

		// ============================================================
		//  EJECUCION DEL ALGORITMO
		// ============================================================
		System::Void btnIniciar_Click(Object^ sender, EventArgs^ e) {
			if (modo == Modo::Ejecucion) {        // "Volver a editar"
				CancelarEjecucion();
				ActualizarUI();
			}
			else IniciarAlgoritmo();
		}

		System::Void cboMetodo_Changed(Object^ sender, EventArgs^ e) {
			if (modo == Modo::Ejecucion) IniciarAlgoritmo();   // se recalcula con el otro metodo
		}

		void IniciarAlgoritmo() {
			if (grafo == nullptr) return;
			DetenerReproduccion();
			bool matricial = cboMetodo->SelectedIndex == 0;
			res = matricial ? Algoritmos::Matricial(grafo) : Algoritmos::Dfs(grafo);
			Resultado^ otro = matricial ? Algoritmos::Dfs(grafo) : Algoritmos::Matricial(grafo);
			coincide = Algoritmos::Iguales(res->componentes, otro->componentes) &&
				Algoritmos::Iguales(Algoritmos::Canonico(res->componentes), Algoritmos::Referencia(grafo));
			nombreOtro = otro->metodo;
			modo = Modo::Ejecucion;
			seleccionado = -1;

			sincronizandoLista = true;
			lstPasos->BeginUpdate();
			lstPasos->Items->Clear();
			for (int k = 0; k < res->pasos->Count; k++)
				lstPasos->Items->Add((k + 1).ToString() + ".  " + res->pasos[k]->titulo);
			lstPasos->EndUpdate();
			sincronizandoLista = false;
			IrAPaso(0);
		}

		void CancelarEjecucion() {
			DetenerReproduccion();
			res = nullptr;
			idx = 0;
			sincronizandoLista = true;
			lstPasos->Items->Clear();
			sincronizandoLista = false;
			if (modo == Modo::Ejecucion) modo = grafo != nullptr ? Modo::Edicion : Modo::SinGrafo;
		}

		void IrAPaso(int k) {
			if (res == nullptr) return;
			idx = Math::Max(0, Math::Min(k, res->pasos->Count - 1));
			sincronizandoLista = true;
			lstPasos->SelectedIndex = idx;
			sincronizandoLista = false;
			if (idx == res->pasos->Count - 1) DetenerReproduccion();
			ActualizarUI();
		}

		void DetenerReproduccion() {
			temporizador->Stop();
			btnReproducir->Text = "▶ Reproducir";
		}

		System::Void btnReiniciar_Click(Object^ sender, EventArgs^ e) { DetenerReproduccion(); IrAPaso(0); }
		System::Void btnAnterior_Click(Object^ sender, EventArgs^ e) { DetenerReproduccion(); IrAPaso(idx - 1); }
		System::Void btnSiguiente_Click(Object^ sender, EventArgs^ e) { IrAPaso(idx + 1); }
		System::Void btnFinal_Click(Object^ sender, EventArgs^ e) { DetenerReproduccion(); if (res != nullptr) IrAPaso(res->pasos->Count - 1); }

		System::Void btnReproducir_Click(Object^ sender, EventArgs^ e) {
			if (res == nullptr) return;
			if (temporizador->Enabled) { DetenerReproduccion(); return; }
			if (idx >= res->pasos->Count - 1) IrAPaso(0);   // si ya termino, vuelve a empezar
			temporizador->Start();
			btnReproducir->Text = "❚❚ Pausa";
		}

		System::Void temporizador_Tick(Object^ sender, EventArgs^ e) {
			if (res == nullptr || idx >= res->pasos->Count - 1) { DetenerReproduccion(); return; }
			IrAPaso(idx + 1);
		}

		System::Void lstPasos_Changed(Object^ sender, EventArgs^ e) {
			if (sincronizandoLista || res == nullptr || lstPasos->SelectedIndex < 0) return;
			DetenerReproduccion();
			IrAPaso(lstPasos->SelectedIndex);
		}

		// Atajos: flechas, espacio, Inicio y Fin recorren los pasos.
	protected:
		virtual bool ProcessCmdKey(Message% msg, Keys keyData) override {
			if (modo == Modo::Ejecucion && !nudN->Focused && !cboMetodo->Focused) {
				switch (keyData) {
				case Keys::Right: case Keys::Space: IrAPaso(idx + 1); return true;
				case Keys::Left: DetenerReproduccion(); IrAPaso(idx - 1); return true;
				case Keys::Home: DetenerReproduccion(); IrAPaso(0); return true;
				case Keys::End: DetenerReproduccion(); IrAPaso(res->pasos->Count - 1); return true;
				default: break;
				}
			}
			return Form::ProcessCmdKey(msg, keyData);
		}
	private:

		// ============================================================
		//  ESTADO DE LA INTERFAZ (unico lugar donde se habilitan controles)
		// ============================================================
		Paso^ PasoActual() { return res != nullptr ? res->pasos[idx] : nullptr; }

		void ActualizarUI() {
			bool hay = grafo != nullptr;
			bool ejec = modo == Modo::Ejecucion && res != nullptr;
			int ultimo = ejec ? res->pasos->Count - 1 : 0;

			btnManual->Enabled = hay;
			btnAleatorio->Enabled = hay;
			btnEjemplo->Enabled = true;
			btnReorganizar->Enabled = hay;
			btnIniciar->Enabled = hay;
			btnIniciar->Text = ejec ? "Volver a editar" : "Iniciar algoritmo";
			btnIniciar->BackColor = ejec ? Tema::Cabecera : Tema::Acento;
			btnIniciar->FlatAppearance->BorderColor = btnIniciar->BackColor;

			btnReiniciar->Enabled = ejec && idx > 0;
			btnAnterior->Enabled = ejec && idx > 0;
			btnSiguiente->Enabled = ejec && idx < ultimo;
			btnReproducir->Enabled = ejec;
			btnFinal->Enabled = ejec && idx < ultimo;
			btnSiguiente->BackColor = btnSiguiente->Enabled ? Tema::Acento : Color::FromArgb(148, 163, 184);
			btnSiguiente->FlatAppearance->BorderColor = btnSiguiente->BackColor;

			if (ejec) {
				Paso^ p = PasoActual();
				lblPasoNum->Text = "Paso" + Environment::NewLine + (idx + 1).ToString() + " / " + res->pasos->Count.ToString();
				lblPasoNum->BackColor = idx == ultimo ? Tema::Verde : Tema::Cabecera;
				lblPasoTitulo->Text = p->titulo->Trim();
				lblPasoTexto->Text = p->texto;
			}
			else if (hay) {
				lblPasoNum->Text = "Editar";
				lblPasoNum->BackColor = Tema::Cabecera;
				lblPasoTitulo->Text = "Construye el grafo de " + grafo->N.ToString() + " nodos";
				lblPasoTexto->Text = "Haz clic en el nodo de origen y luego en el de destino para crear o quitar la flecha (o clic en la celda i, j "
					"de la matriz). Dos clics en el mismo nodo crean un lazo. Cuando el grafo esté listo, elige el método y pulsa «Iniciar algoritmo».";
			}
			else {
				lblPasoNum->Text = "Inicio";
				lblPasoNum->BackColor = Tema::Cabecera;
				lblPasoTitulo->Text = "Elige la cantidad de nodos (4 a 12) y pulsa «Crear grafo»";
				lblPasoTexto->Text = "También puedes cargar el «Ejemplo del informe» para comparar el programa con la resolución manual.";
			}
			lienzoGrafo->Invalidate();
			lienzoMatriz->Invalidate();
			lienzoResultado->Invalidate();
			lstPasos->Invalidate();
		}

		// ============================================================
		//  DIBUJO
		// ============================================================
		System::Void lienzoGrafo_Paint(Object^ sender, PaintEventArgs^ e) {
			String^ estado = "";
			if (grafo != nullptr) {
				estado = "n = " + grafo->N.ToString() + "  ·  " + grafo->ContarAristas().ToString() + (grafo->Dirigido ? " arcos" : " aristas");
				if (grafo->ContarLazos() > 0) estado += " (" + grafo->ContarLazos().ToString() + " lazos)";
				if (modo == Modo::Edicion && seleccionado >= 0)
					estado += "  ·  nodo " + (seleccionado + 1).ToString() + " elegido: clic en el destino (o en él mismo: lazo)";
				else if (modo == Modo::Ejecucion && res != nullptr)
					estado += "  ·  " + res->metodo;
			}
			Render::DibujarGrafo(e->Graphics, lienzoGrafo->ClientRectangle, grafo, pos, PasoActual(),
				seleccionado, hoverNodo, estado);
		}

		System::Void lienzoMatriz_Paint(Object^ sender, PaintEventArgs^ e) {
			bool fin = res != nullptr && idx == res->pasos->Count - 1;
			Render::DibujarMatriz(e->Graphics, lienzoMatriz->ClientRectangle, grafo, PasoActual(),
				cboMetodo->SelectedIndex == 0, fin, hoverI, hoverJ, modo == Modo::Edicion);
		}

		System::Void lienzoResultado_Paint(Object^ sender, PaintEventArgs^ e) {
			Render::DibujarResultado(e->Graphics, lienzoResultado->ClientRectangle, res, idx, coincide, nombreOtro);
		}

		System::Void lstPasos_DrawItem(Object^ sender, DrawItemEventArgs^ e) {
			if (e->Index < 0 || res == nullptr || e->Index >= res->pasos->Count) return;
			bool actual = e->Index == idx;
			bool futuro = e->Index > idx;
			Color fondo = actual ? Tema::AcentoSuave : Color::White;
			SolidBrush bf(fondo);
			e->Graphics->FillRectangle(%bf, e->Bounds);
			if (actual) {
				SolidBrush ba(Tema::Acento);
				e->Graphics->FillRectangle(%ba, e->Bounds.X, e->Bounds.Y, 4, e->Bounds.Height);
			}
			Paso^ p = res->pasos[e->Index];
			if (p->tipo == TipoPaso::Bloque && !futuro) {
				int c = 0;
				for (int t = 0; t < e->Index; t++) if (res->pasos[t]->tipo == TipoPaso::Bloque) c++;
				SolidBrush bc(Tema::ColorComp(c));
				e->Graphics->FillEllipse(%bc, e->Bounds.Right - 22, e->Bounds.Y + 7, 10, 10);
			}
			Color ct = futuro ? Color::FromArgb(176, 186, 200) : Tema::Texto;
			TextRenderer::DrawText(e->Graphics, lstPasos->Items[e->Index]->ToString(), actual ? Tema::Negrita : Tema::Normal,
				Rectangle(e->Bounds.X + 10, e->Bounds.Y, e->Bounds.Width - 36, e->Bounds.Height), ct,
				TextFormatFlags::VerticalCenter | TextFormatFlags::EndEllipsis | TextFormatFlags::Left);
		}

		// ============================================================
		//  MOUSE: grafo (seleccionar / arrastrar) y matriz (alternar celda)
		// ============================================================
		System::Void lienzoGrafo_MouseDown(Object^ sender, MouseEventArgs^ e) {
			if (e->Button != System::Windows::Forms::MouseButtons::Left) return;
			arrastrado = Render::NodoEn(e->Location, lienzoGrafo->ClientRectangle, pos);
			inicioArrastre = e->Location;
			seMovio = false;
		}

		System::Void lienzoGrafo_MouseMove(Object^ sender, MouseEventArgs^ e) {
			if (arrastrado >= 0 && e->Button == System::Windows::Forms::MouseButtons::Left) {
				if (Math::Abs(e->X - inicioArrastre.X) + Math::Abs(e->Y - inicioArrastre.Y) > 4) seMovio = true;
				if (seMovio) {
					Rectangle area = Render::AreaGrafo(lienzoGrafo->ClientRectangle);
					pos[arrastrado] = Render::ANormal(e->Location, area, Render::Radio(area, pos->Length) + 8);
					lienzoGrafo->Invalidate();
				}
				return;
			}
			int h = Render::NodoEn(e->Location, lienzoGrafo->ClientRectangle, pos);
			lienzoGrafo->Cursor = h >= 0 ? Cursors::Hand : Cursors::Default;
			if (h != hoverNodo) { hoverNodo = h; lienzoGrafo->Invalidate(); }
		}

		System::Void lienzoGrafo_MouseUp(Object^ sender, MouseEventArgs^ e) {
			if (e->Button != System::Windows::Forms::MouseButtons::Left) return;
			int nodo = arrastrado;
			arrastrado = -1;
			if (seMovio || modo != Modo::Edicion) return;   // fue un arrastre o no se puede editar
			if (nodo < 0) { seleccionado = -1; lienzoGrafo->Invalidate(); return; }
			if (seleccionado == -1) seleccionado = nodo;
			else {
				AlternarArista(seleccionado, nodo);   // si es el mismo nodo, crea / quita el lazo
				seleccionado = -1;
			}
			lienzoGrafo->Invalidate();
		}

		System::Void lienzoMatriz_MouseMove(Object^ sender, MouseEventArgs^ e) {
			if (grafo == nullptr || modo != Modo::Edicion) return;
			int i = -1, j = -1;
			if (!Render::CeldaEn(e->Location, lienzoMatriz->ClientRectangle, grafo->N, i, j)) { i = -1; j = -1; }
			lienzoMatriz->Cursor = i >= 0 ? Cursors::Hand : Cursors::Default;
			if (i != hoverI || j != hoverJ) { hoverI = i; hoverJ = j; lienzoMatriz->Invalidate(); }
		}

		System::Void lienzoMatriz_MouseLeave(Object^ sender, EventArgs^ e) {
			hoverI = hoverJ = -1;
			lienzoMatriz->Invalidate();
		}

		System::Void lienzoMatriz_MouseClick(Object^ sender, MouseEventArgs^ e) {
			if (grafo == nullptr || modo != Modo::Edicion) return;
			int i, j;
			if (Render::CeldaEn(e->Location, lienzoMatriz->ClientRectangle, grafo->N, i, j))
				AlternarArista(i, j);
		}

		// ============================================================
		//  INICIO DEL FORMULARIO Y CAPTURAS AUTOMATICAS
		// ============================================================
		System::Void MyForm_Load(Object^ sender, EventArgs^ e) {
			division->SplitterDistance = (int)(division->Width * 0.5);
		}

		System::Void MyForm_Shown(Object^ sender, EventArgs^ e) {
			if (!String::IsNullOrEmpty(carpetaCapturas)) {
				GenerarCapturas(carpetaCapturas);
				Close();
			}
			else if (accionesEstres > 0) {
				codigoSalida = PruebaEstres(accionesEstres);
				Close();
			}
		}

		void Guardar(Control^ c, String^ archivo) {
			Application::DoEvents();
			Bitmap bmp(c->Width, c->Height);
			c->DrawToBitmap(%bmp, Rectangle(0, 0, c->Width, c->Height));
			bmp.Save(IO::Path::Combine(carpetaCapturas, archivo), Imaging::ImageFormat::Png);
		}

		void CapturarPaso(int k, String^ prefijo) {
			IrAPaso(k);
			String^ nn = (k + 1).ToString("00");
			Guardar(raiz, prefijo + "_" + nn + "_app.png");
			Guardar(lienzoGrafo, prefijo + "_" + nn + "_grafo.png");
			Guardar(lienzoMatriz, prefijo + "_" + nn + "_matriz.png");
		}

		void Invariante(bool cond, String^ msg) {
			if (!cond) throw gcnew InvalidOperationException("Invariante violada: " + msg);
		}

		void VerificarEstado() {
			bool ejec = modo == Modo::Ejecucion;
			Invariante(!ejec || res != nullptr, "ejecucion sin resultado");
			Invariante(ejec || res == nullptr, "resultado fuera de ejecucion");
			Invariante(modo != Modo::SinGrafo || grafo == nullptr, "SinGrafo con grafo");
			Invariante(grafo == nullptr || pos->Length == grafo->N, "posiciones != n");
			Invariante(grafo == nullptr || Decimal::ToInt32(nudN->Value) == grafo->N, "nudN distinto de n");
			if (ejec) {
				Invariante(idx >= 0 && idx < res->pasos->Count, "indice de paso fuera de rango");
				Invariante(lstPasos->Items->Count == res->pasos->Count, "lista de pasos desincronizada");
				Invariante(lstPasos->SelectedIndex == idx, "seleccion de la lista != paso");
				Invariante(btnSiguiente->Enabled == (idx < res->pasos->Count - 1), "Siguiente mal habilitado");
				Invariante(btnAnterior->Enabled == (idx > 0), "Anterior mal habilitado");
				Invariante(coincide, "los metodos no coinciden");
			}
			else {
				Invariante(!btnSiguiente->Enabled && !btnAnterior->Enabled, "navegacion habilitada sin ejecucion");
				Invariante(!temporizador->Enabled, "temporizador activo sin ejecucion");
			}
			for (int i = 0; grafo != nullptr && i < grafo->N; i++)
				for (int j = 0; j < grafo->N; j++) Invariante(grafo->Dirigido || grafo->Ady(i, j) == grafo->Ady(j, i), "grafo no dirigido con matriz no simetrica");
			Invariante(grafo == nullptr || grafo->Dirigido, "el grafo debe ser dirigido");
			if (ejec) Invariante(res->caminos != nullptr && res->orden != nullptr, "faltan la matriz de caminos o el orden");
		}

		void ClicSimulado(Control^ c, Point pt) {
			MouseEventArgs^ e = gcnew MouseEventArgs(System::Windows::Forms::MouseButtons::Left, 1, pt.X, pt.Y, 0);
			if (c == lienzoGrafo) { lienzoGrafo_MouseDown(c, e); lienzoGrafo_MouseMove(c, e); lienzoGrafo_MouseUp(c, e); }
			else { lienzoMatriz_MouseMove(c, e); lienzoMatriz_MouseClick(c, e); }
		}

		int PruebaEstres(int acciones) {
			Random^ r = gcnew Random(2026);
			String^ ultima = "";
			try {
				for (int k = 0; k < acciones; k++) {
					int a = r->Next(20);
					ultima = "accion " + a.ToString() + " (iteracion " + k.ToString() + ")";
					switch (a) {
					case 0: nudN->Value = r->Next(4, 13); btnCrear->PerformClick(); break;
					case 1: btnManual->PerformClick(); break;
					case 2: case 3: btnAleatorio->PerformClick(); break;
					case 4: btnEjemplo->PerformClick(); break;
					case 5: btnReorganizar->PerformClick(); break;
					case 6: case 7: btnIniciar->PerformClick(); break;
					case 8: cboMetodo->SelectedIndex = r->Next(2); break;
					case 9: case 10: btnSiguiente->PerformClick(); break;
					case 11: btnAnterior->PerformClick(); break;
					case 12: btnReiniciar->PerformClick(); btnFinal->PerformClick(); break;
					case 13: btnReproducir->PerformClick(); temporizador_Tick(nullptr, nullptr); temporizador_Tick(nullptr, nullptr); break;
					case 14: if (lstPasos->Items->Count > 0) lstPasos->SelectedIndex = r->Next(lstPasos->Items->Count); break;
					case 15:   // clic sobre un nodo (o al lado)
						if (grafo != nullptr) {
							Rectangle area = Render::AreaGrafo(lienzoGrafo->ClientRectangle);
							PointF c = Render::APixel(pos[r->Next(grafo->N)], area, Render::Radio(area, grafo->N) + 8);
							ClicSimulado(lienzoGrafo, Point((int)c.X + r->Next(-30, 31), (int)c.Y + r->Next(-30, 31)));
						}
						break;
					case 16: ClicSimulado(lienzoMatriz, Point(r->Next(lienzoMatriz->Width), r->Next(lienzoMatriz->Height))); break;
					case 18: if (grafo != nullptr) AlternarArista(r->Next(grafo->N), r->Next(grafo->N)); break;
					case 19: if (grafo != nullptr) { int v = r->Next(grafo->N); AlternarArista(v, v); } break;
					case 17: this->WindowState = FormWindowState::Normal; this->Size = Drawing::Size(r->Next(1240, 1600), r->Next(780, 1000)); break;
					}
					VerificarEstado();
					if (k % 7 == 0) this->Refresh();   // fuerza el dibujo completo
				}
			}
			catch (Exception^ ex) {
				Console::Error->WriteLine("FALLO en " + ultima + ": " + ex->ToString());
				IO::File::WriteAllText("estres_error.txt", ultima + Environment::NewLine + ex->ToString());
				return 1;
			}
			return 0;
		}

		void GenerarCapturas(String^ carpeta) {
			IO::Directory::CreateDirectory(carpeta);
			Guardar(raiz, "00_inicio.png");
			// Ejemplo del informe, metodo matricial: todos los pasos
			CargarEjemplo();
			ActualizarUI();
			Guardar(raiz, "01_ejemplo_edicion.png");
			Guardar(lienzoMatriz, "01_ejemplo_matriz_A.png");
			IniciarAlgoritmo();
			for (int k = 0; k < res->pasos->Count; k++) CapturarPaso(k, "mat");
			Guardar(lienzoResultado, "mat_resultado.png");
			// Mismo ejemplo con DFS
			cboMetodo->SelectedIndex = 1;   // reinicia con DFS
			for (int k = 0; k < res->pasos->Count; k++) CapturarPaso(k, "dfs");
			// Construccion manual: nodo seleccionado
			CancelarEjecucion();
			cboMetodo->SelectedIndex = 0;
			nudN->Value = 6;
			NuevoGrafo(6);
			grafo->PonerArista(0, 1);
			grafo->PonerArista(1, 2);
			grafo->PonerArista(2, 1);
			grafo->PonerArista(4, 4);
			seleccionado = 3;
			ActualizarUI();
			Guardar(raiz, "02_manual.png");
			// Grafo aleatorio de 12 nodos
			rnd = gcnew Random(11);
			nudN->Value = 12;
			NuevoGrafo(12);
			GenerarAleatorio();
			ActualizarUI();
			Guardar(raiz, "03_aleatorio12.png");
			IniciarAlgoritmo();
			IrAPaso(6);
			Guardar(raiz, "03_aleatorio12_paso.png");
			IrAPaso(res->pasos->Count - 1);
			Guardar(raiz, "03_aleatorio12_final.png");
			// Casos limite: grafo sin aristas y grafo completo
			nudN->Value = 4;
			NuevoGrafo(4);
			IniciarAlgoritmo();
			IrAPaso(res->pasos->Count - 1);
			Guardar(raiz, "04_vacio4_final.png");
			NuevoGrafo(12);
			for (int i = 0; i < 12; i++) for (int j = i + 1; j < 12; j++) grafo->PonerArista(i, j);
			IniciarAlgoritmo();
			IrAPaso(res->pasos->Count - 1);
			Guardar(raiz, "05_completo12_final.png");
			// Grafo dirigido aleatorio de 9 nodos
			rnd = gcnew Random(3);
			nudN->Value = 9;
			NuevoGrafo(9);
			GenerarAleatorio();
			IniciarAlgoritmo();
			IrAPaso(res->pasos->Count - 1);
			Guardar(raiz, "07_dirigido9_final.png");
		}
	};
}
