#include "MyForm.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
int main(array<String^>^ args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	ComponentesConexas::MyForm form;
	// Uso interno: "ComponentesConexas.exe --capturas <carpeta>" guarda las imagenes del informe.
	if (args->Length >= 2 && args[0] == "--capturas") form.ProgramarCapturas(args[1]);
	if (args->Length >= 2 && args[0] == "--estres") form.ProgramarEstres(Int32::Parse(args[1]));
	Application::Run(%form);

	return form.CodigoSalida;
}
