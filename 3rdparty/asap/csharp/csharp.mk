CSC = $(DO)"C:/Windows/Microsoft.NET/Framework/v2.0.50727/csc.exe" -nologo -o+ -out:$@ $(if $(filter %.dll,$@),-t:library) $(subst /,\\,$^)
NDOC = $(DO)"C:/Program Files (x86)/NDoc3/bin/NDoc3Console.exe"
SL = "C:/Program Files (x86)/Reference Assemblies/Microsoft/Framework/Silverlight/v3.0"

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "csharp.mk")
endif

csharp: csharp/asap2wav.exe csharp/asapplay.exe csharp/SilverASAP.xap
.PHONY: csharp

csharp/asap2wav.exe: $(srcdir)csharp/asap2wav.cs csharp/asap.cs
	$(CSC)
CLEAN += csharp/asap2wav.exe

csharp/asapplay.exe: $(srcdir)csharp/asapplay.cs csharp/asap.cs
	$(CSC)
CLEAN += csharp/asapplay.exe

csharp/SilverASAP.xap: csharp/SilverASAP.dll $(srcdir)csharp/AppManifest.xaml
	$(MAKEZIP)
CLEAN += csharp/SilverASAP.xap

csharp/SilverASAP.dll: $(srcdir)csharp/SilverASAP.cs csharp/asap.cs
	$(CSC) -nostdlib -noconfig -r:$(SL)/mscorlib.dll -r:$(SL)/system.dll -r:$(SL)/System.Net.dll -r:$(SL)/System.Windows.dll -r:$(SL)/System.Windows.Browser.dll
CLEAN += csharp/SilverASAP.dll

csharp/asap.cs: $(call src,asap.ci asap6502.ci asapinfo.ci cpu6502.ci pokey.ci) $(ASM6502_PLAYERS_OBX)
	$(CITO) -n Sf.Asap
CLEAN += csharp/asap.cs

csharp/doc/ASAP.chm: csharp/doc/ASAP.dll
	$(NDOC) -documenter:MSDN -CleanIntermediates=true -DocumentInheritedFrameworkMembers=false \
		-OutputDirectory=csharp/doc -OutputTarget=HtmlHelp -HtmlHelpName=ASAP -Title="ASAP .NET API" $<
CLEAN += csharp/doc/ASAP.chm

csharp/doc/ASAP.dll: csharp/asap.cs
	$(CSC) -t:library -doc:csharp/doc/ASAP.xml
CLEAN += csharp/doc/ASAP.dll csharp/doc/ASAP.xml

include $(srcdir)csharp/winrt/winrt.mk
