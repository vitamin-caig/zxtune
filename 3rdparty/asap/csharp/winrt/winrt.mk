CSC45 = $(DO)"C:/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe" -nologo -o+ -out:$@ -t:appcontainerexe $(subst /,\\,$^)
WIN8_SDK = "C:/Program Files (x86)/Windows Kits/8.0"
MAKEAPPX = $(DO)$(WIN8_SDK)/bin/x86/makeappx.exe
NET45_RA = "C:/Program Files (x86)/Reference Assemblies/Microsoft/Framework/.NETCore/v4.5"
CONVERT = $(DO)convert

# no user-configurable paths below this line

ifndef DO
$(error Use "Makefile" instead of "winrt.mk")
endif

csharp/winrt/MetroASAP.appx: csharp/winrt/obj/MetroASAP.exe csharp/winrt/obj/AppxManifest.xml csharp/winrt/obj/Logo.png csharp/winrt/obj/SmallLogo.png csharp/winrt/obj/SplashScreen.png csharp/winrt/obj/FileLogo.png
	$(MAKEAPPX) pack -d csharp/winrt/obj -o -p $@
CLEAN += csharp/winrt/MetroASAP.appx

csharp/winrt/obj/MetroASAP.exe: $(srcdir)csharp/winrt/MetroASAP.cs csharp/asap.cs
	$(CSC45) -nostdlib -noconfig -r:$(WIN8_SDK)/References/CommonConfiguration/Neutral/Windows.winmd \
		-r:$(NET45_RA)/mscorlib.dll -r:$(NET45_RA)/System.Diagnostics.Debug.dll -r:$(NET45_RA)/System.Runtime.dll \
		-r:$(NET45_RA)/System.Runtime.Extensions.dll -r:$(NET45_RA)/System.Runtime.InteropServices.WindowsRuntime.dll \
		-r:$(NET45_RA)/System.Runtime.WindowsRuntime.dll -r:$(NET45_RA)/System.Text.Encoding.dll -r:$(NET45_RA)/System.Threading.Tasks.dll
CLEANDIR += csharp/winrt/obj

csharp/winrt/obj/AppxManifest.xml: $(srcdir)csharp/winrt/AppxManifest.xml
	$(COPY)

csharp/winrt/obj/Logo.png: $(srcdir)csharp/winrt/logo.svg
	$(CONVERT) -background none -density 600 $< -gravity Center -resize "75x75!" -extent 150x150 -quality 95 $@

csharp/winrt/obj/SmallLogo.png: $(srcdir)csharp/winrt/logo.svg
	$(CONVERT) -background none -density 600 $< -gravity Center -resize "28x28!" -extent 30x30 -quality 95 $@

csharp/winrt/obj/SplashScreen.png: $(srcdir)csharp/winrt/logo.svg
	$(CONVERT) -background none -density 600 $< -gravity Center -resize "280x280!" -extent 620x300 -quality 95 $@

csharp/winrt/obj/FileLogo.png: $(srcdir)csharp/winrt/logo.svg
	$(CONVERT) -background "#c02020" -density 600 $< -gravity Center -resize "36x36!" -extent 48x48 -quality 95 $@
