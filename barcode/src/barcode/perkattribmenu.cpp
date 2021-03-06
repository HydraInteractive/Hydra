#include <barcode/perkattribmenu.hpp>

PerkAttribMenu::PerkAttribMenu()
{

}

PerkAttribMenu::~PerkAttribMenu()
{
}
void PerkAttribMenu::render(bool & thing, Hydra::Renderer::Batch * renderBatch, float delta)
{
	ImGui::SetNextWindowSize(ImVec2(500, 1000), ImGuiCond_Once);
	ImGui::Begin("Attributes", &thing, ImGuiWindowFlags_MenuBar);

	//Whatever you choose in the editor will be added/multiplied by
	//whatever values the player already has
	
	ImGui::RadioButton("Bullet", &Bullet.mesh, 0); ImGui::SameLine();
	ImGui::RadioButton("Star", &Bullet.mesh, 1); ImGui::SameLine();
	ImGui::RadioButton("Trident", &Bullet.mesh, 2); ImGui::SameLine();
	ImGui::RadioButton("Banana", &Bullet.mesh, 3); ImGui::SameLine();
	ImGui::RadioButton("Duck", &Bullet.mesh, 4);

	ImGui::Separator();
	ImGui::Checkbox("Glow on/off", &Bullet.glow);
	ImGui::Separator();
	ImGui::Text("Glow Intensity:"); ImGui::SameLine(); ImGui::DragFloat("                             ", &Bullet.glowIntensity, 0.01f);
	ImGui::Separator();
	ImGui::Text("Bullet Size:"); ImGui::SameLine(); ImGui::DragFloat("", &Bullet.bulletSize, 0.01f);
	ImGui::Separator();
	ImGui::Text("Bullet Damage:"); ImGui::SameLine(); ImGui::DragFloat("  ",&Bullet.dmg, 0.01f);
	ImGui::Separator();
	ImGui::Text("Weapon Recoil:"); ImGui::SameLine(); ImGui::DragFloat("   ", &Bullet.recoil, 0.01f);
	ImGui::Separator();
	//if (ImGui::IsItemHovered())
	//{
	//	ImGui::SetTooltip("This will add the amount of ammo that will fit in your magazine");
	//}
	ImGui::Text("Mag Ammo:"); ImGui::SameLine(); ImGui::DragInt("      ", &Bullet.currentMagAmmo);
	ImGui::Separator();
	//if (ImGui::IsItemHovered())
	//{
	//	ImGui::SetTooltip("This will make the shots go in dirrenrent direction");
	//}
	ImGui::Text("Shot Spread:"); ImGui::SameLine(); ImGui::DragFloat("       ", &Bullet.bulletSpread, 0.01f);
	ImGui::Separator();
	//if (ImGui::IsItemHovered())
	//{
	//	ImGui::SetTooltip("This will add the amount of shots you'll fire per minute");
	//}
	ImGui::Text("Shot RPM:"); ImGui::SameLine(); ImGui::DragFloat("          ", &Bullet.roundsPerMinute, 0.01f);
	ImGui::Separator();
	ImGui::ColorPicker4("Bullet Color:",(float*)&Bullet.bulletColor,ImGuiColorEditFlags_RGB);
	ImGui::Separator();
	ImGui::Text("Perk Description:");
	ImGui::InputText("                            ", Bullet.perkDescription, 128);
	ImGui::Separator();
	//ImGui::RadioButton("Multiplier", &Bullet.multi); ImGui::SameLine(); ImGui::RadioButton("Adder", &Bullet.add);
	ImGui::Checkbox("Multiplier", &Bullet.Multiplier), ImGui::SameLine(); ImGui::Checkbox("Adder", &Bullet.Adder);
	//ImGui::Checkbox("Bullet Mesh", )
	//if (Bullet.mesh != lastMesh) {
	//
	//}

	if (ImGui::Button("Export", ImVec2(200, 75)))
	{
		openPopup = true;
	}

	if (openPopup)
	{
		ImGui::OpenPopup("Exporter");
		if (ImGui::BeginPopup("Exporter"))
		{
			ImGui::Text("Write the name of your Perk:");
			ImGui::InputText("  ", Bullet.perkName, 128);

			if (ImGui::Button("Done", ImVec2(200, 75)))
			{
				writeToFile(Bullet.perkName);
				openPopup = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Exit", ImVec2(50, 75)))
			{
				openPopup = false;
			}
			ImGui::EndPopup();
		}
	}
		

	ImGui::End();
}

void PerkAttribMenu::updateBullet(BulletInfo& bullet)
{
	bullet = Bullet;
}

void PerkAttribMenu::writeToFile(const char* fileName)
{
	//file << Bullet.bulletSize;
	//file << Bullet.dmg;
	//file << Bullet.recoil;
	//file << Bullet.currentMagAmmo;
	//file << Bullet.bulletSpread;
	//file << Bullet.roundsPerMinute;
	//file << Bullet.bulletColor[0];
	//file << Bullet.bulletColor[1];
	//file << Bullet.bulletColor[2];
	//file.write(reinterpret_cast<char *>(&Bullet.perkDescription), sizeof(char));
	//binary
	//file << Bullet.perkDescription;
	
	
	std::string name = fileName;
	std::string filepath = "assets/perks/" + name + ".PERK";
	std::ofstream file;
	file.open(filepath, std::ios::binary);
	
	file.write(reinterpret_cast<char *>(&Bullet.bulletSize), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.dmg), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.recoil), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.currentMagAmmo), sizeof(int));
	file.write(reinterpret_cast<char *>(&Bullet.bulletSpread), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.roundsPerMinute), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.bulletColor[0]), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.bulletColor[1]), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.bulletColor[2]), sizeof(float));
	file.write(reinterpret_cast<char *>(&Bullet.Adder), sizeof(bool));
	file.write(reinterpret_cast<char *>(&Bullet.Multiplier), sizeof(bool));
	file.write(reinterpret_cast<char *>(&Bullet.glow), sizeof(bool));
	file.write(reinterpret_cast<char *>(&Bullet.glowIntensity), sizeof(float));
	file.write(reinterpret_cast<char *> (&Bullet.mesh), sizeof(int));

	std::string description = Bullet.perkDescription;
	int size = description.size();

	file.write(reinterpret_cast<char*>(&size), sizeof(int));
	file.write(description.data(), size);

	file.close();
}




