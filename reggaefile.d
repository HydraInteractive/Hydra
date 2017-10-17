import reggae;
import std.algorithm;
import std.array;
import std.conv;

enum string[] SubProjects = ["hydra", "hydra_graphics", "hydra_network", "hydra_physics", "hydra_sound"];
enum string SubProjectsInclude = SubProjects.map!((string x) => "-I" ~ x ~ "/include").joiner(" ").array.to!string;
enum string SubProjectsLink = SubProjects.map!((string x) => "-l" ~ x).joiner(" ").array.to!string;

enum warnings = "-Wall -Wextra -Werror -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wformat=2 -Wno-error=unused-parameter -Wno-error=format-nonliteral -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=reorder";

enum string CFlagsLib = "-O0 -std=c++1z -ffat-lto-objects -ggdb " ~ warnings ~ " -fdiagnostics-color=always -fopenmp -fPIC " ~ SubProjectsInclude;

enum string CFlagsHydraBaseLib = "-DHYDRA_BASE_EXPORTS " ~ CFlagsLib;
enum string CFlagsHydraGraphicsLib = "-DHYDRA_GRAPHICS_EXPORTS " ~ CFlagsLib;
enum string CFlagsHydraNetworkLib = "-DHYDRA_NETWORK_EXPORTS " ~ CFlagsLib;
enum string CFlagsHydraPhysicsLib = "-DHYDRA_PHYSICS_EXPORTS " ~ CFlagsLib;
enum string CFlagsHydraSoundLib = "-DHYDRA_SOUND_EXPORTS " ~ CFlagsLib;
enum string CFlagsExec = "-DBARCODE_EXPORTS -O0 -std=c++1z -ffat-lto-objects -ggdb " ~ warnings ~ " -fdiagnostics-color=always -fopenmp -fPIC -Ibarcode/include " ~ SubProjectsInclude;

enum LFlagsHydraBaseLib = "-O0 -shared -Wl,--no-undefined -Wl,-rpath,objs/barcodegame.objs -Lobjs/barcodegame.objs -ggdb -fdiagnostics-color=always -fopenmp -lm -ldl -lSDL2";
enum LFlagsHydraGraphicsLib = "-O0 -shared -Wl,--no-undefined -Wl,-rpath,objs/barcodegame.objs -Lobjs/barcodegame.objs -ggdb -fdiagnostics-color=always -fopenmp -ldl -lhydra -lGL -lSDL2 -lSDL2_image -lSDL2_ttf";
enum LFlagsHydraNetworkLib = "-O0 -shared -Wl,--no-undefined -Wl,-rpath,objs/barcodegame.objs -Lobjs/barcodegame.objs -ggdb -fdiagnostics-color=always -fopenmp -lhydra -lhydra_graphics -lSDL2_net";
enum LFlagsHydraPhysicsLib = "-O0 -shared -Wl,--no-undefined -Wl,-rpath,objs/barcodegame.objs -Lobjs/barcodegame.objs -ggdb -fdiagnostics-color=always -fopenmp -lhydra -lhydra_graphics -lSDL2 -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath";
enum LFlagsHydraSoundLib = "-O0 -shared -Wl,--no-undefined -Wl,-rpath,objs/barcodegame.objs -Lobjs/barcodegame.objs -ggdb -fdiagnostics-color=always -fopenmp -lhydra -lhydra_graphics -lSDL2_mixer";
enum LFlagsExec = "-O0 -rdynamic -Wl,--no-undefined -Wl,-rpath,objs/barcodegame.objs -Lobjs/barcodegame.objs -ggdb -fdiagnostics-color=always -fopenmp -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath " ~ SubProjectsLink;

enum CompileCommand : string {
	CompileExec = "g++ -c " ~ CFlagsExec ~ " $in -o $out",
	LinkExec = "g++ " ~ LFlagsExec ~ " $in -o $out"
}
enum string Compile(string lib) = "g++ -c " ~ lib ~ " $in -o $out";
enum string Link(string lib) = "g++ " ~ lib ~ " $in -o $out";

Target[] MakeObjects(string src, CompileCommand cmd)() {
	import std.file : dirEntries, SpanMode;
	import std.process : executeShell;
	import std.algorithm : map;
	import std.array : array, replace, split;
	import std.range : chain;
	import std.stdio : writeln;

	Target[] objs;

	foreach (f; chain(dirEntries(src, "*.cpp", SpanMode.breadth), dirEntries(src, "*.c", SpanMode.breadth)).filter!(x => !x.isDir)) {
		auto exec = executeShell("g++ -MM " ~ CFlagsLib ~ " " ~ f);
		if (exec.status) {
			import std.stdio : stderr;

			stderr.writeln("Returned: ", exec.status, "\n", exec.output);
			assert(0);
		}

		auto head = exec.output.split(":")[1].replace("\n", " ").split(" ").filter!(s => !s.empty && s != "\\").map!(x => Target(x)).array[1 .. $];
		//writeln(f, " needs: ", head);
		objs ~= Target(f ~ ".o", cmd, [Target(f)], head);
	}

	return objs;
}

Build myBuild() {
	auto libhydra = Target("libhydra.so", Link!(LFlagsHydraBaseLib), MakeObjects!("hydra/src/", Compile!(CFlagsHydraBaseLib)));
	auto libhydra_graphics = Target("libhydra_graphics.so", Link!(LFlagsHydraGraphicsLib), MakeObjects!("hydra_graphics/src/", Compile!(CFlagsHydraGraphicsLib)), [libhydra]);
	auto libhydra_network = Target("libhydra_network.so", Link!(LFlagsHydraNetworkLib), MakeObjects!("hydra_network/src/", Compile!(CFlagsHydraNetworkLib)), [libhydra, libhydra_graphics]);
	auto libhydra_physics = Target("libhydra_physics.so", Link!(LFlagsHydraPhysicsLib), MakeObjects!("hydra_physics/src/", Compile!(CFlagsHydraPhysicsLib)), [libhydra, libhydra_graphics]);
	auto libhydra_sound = Target("libhydra_sound.so", Link!(LFlagsHydraSoundLib), MakeObjects!("hydra_sound/src/", Compile!(CFlagsHydraSoundLib)), [libhydra, libhydra_graphics]);
	auto barcode = Target("barcodegame", CompileCommand.LinkExec, MakeObjects!("barcode/src/", CompileCommand.CompileExec), [libhydra, libhydra_graphics, libhydra_network, libhydra_physics, libhydra_sound]);

	return Build(barcode);
}
