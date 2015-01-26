#include "systems/vcomputer-system.hpp"

#include "trillek.hpp"

#include "vc.hpp"

namespace trillek {

using namespace computer;
VComputerSystem::VComputerSystem() {
    this->gkeyb = std::make_shared<gkeyboard::GKeyboardDev>();

    AddComputer(9000, CPU_TYPE::TR3200);
    if (LoadROMFile(9000, "common/trillek-vcomputer-module/asm/type1.ffi")) {
        this->pixelBuffers[9000].first = resource::ResourceMap::Get<resource::PixelBuffer>("1005_common/assets/vidstand/Screen.png");
        this->pixelBuffers[9000].first->Create(320, 240, 8, resource::ImageColorMode::COLOR_RGBA);
        this->pixelBuffers[9000].second = std::make_shared<tda::TDADev>();
        SetDevice(9000, 5, this->pixelBuffers[9000].second);
        SetDevice(9000, 1, gkeyb);
        TurnComptuerOn(9000);
    }
    else {
        RemoveComputer(9000);
    }
    /*
    AddComputer(9001, DCPUN);
    if (LoadROMFile(9001, "common/trillek-vcomputer-module/asm/dcpu16n/hello.ffi")) {
        this->pixelBuffers[9001].first = resource::ResourceMap::Get<resource::PixelBuffer>("1004_common/assets/vidstand/Screen.png");
        this->pixelBuffers[9001].first->Create(320, 240, 8, resource::ImageColorMode::COLOR_RGBA);
        this->pixelBuffers[9001].second = std::make_shared<tda::TDADev>();
        SetDevice(9001, 5, this->pixelBuffers[9001].second);
        SetDevice(9001, 1, gkeyb);
        TurnComptuerOn(9001);
    }
    else {
        RemoveComputer(9001);
    }
    */
    event::Dispatcher<KeyboardEvent>::GetInstance()->Subscribe(this);
};

VComputerSystem::~VComputerSystem() { }

void VComputerSystem::AddComputer(const id_t entity_id, CPU_TYPE type) {
    std::unique_ptr<VComputer> vc(new VComputer());
    if (type == CPU_TYPE::TR3200) {
        std::unique_ptr<computer::TR3200> trcpu(new computer::TR3200());
        vc->SetCPU(std::move(trcpu));
        this->computers[entity_id].vc = std::move(vc);
    }
    /*
    else if (type == DCPUN) {
        std::unique_ptr<cpu::DCPU16N> dcpu(new cpu::DCPU16N());
        vc->SetCPU(std::move(dcpu));
        this->computers[entity_id].vc = std::move(vc);
    }
    */
}

void VComputerSystem::SetDevice(const id_t entity_id, const unsigned int slot, std::shared_ptr<Device> device) {
    if (this->computers.find(entity_id) != this->computers.end()) {
        this->computers[entity_id].vc->AddDevice(slot, device);
        this->computers[entity_id].devices.push_back(device);
    }
}

void VComputerSystem::RemoveComputer(const id_t entity_id) {
    if (this->computers.find(entity_id) != this->computers.end()) {
        this->computers[entity_id].vc->Off();
        this->computers.erase(entity_id);
    }
}

void VComputerSystem::RemoveDevice(const id_t entity_id, const unsigned int slot) {
    if (this->computers.find(entity_id) != this->computers.end()) {
        auto dev = this->computers[entity_id].vc->GetDevice(slot);
        for (auto itr = this->computers[entity_id].devices.begin(); itr != this->computers[entity_id].devices.end(); ++itr) {
            if ((*itr) == dev) {
                this->computers[entity_id].devices.erase(itr);
                if (this->pixelBuffers.find(entity_id) != this->pixelBuffers.end()) {
                    if ((*itr) == this->pixelBuffers[entity_id].second) {
                        this->pixelBuffers.erase(entity_id);
                    }
                }
                this->computers[entity_id].devices.erase(itr);
                break;
            }
        }
        this->computers[entity_id].vc->RmDevice(slot);
    }
}

void VComputerSystem::HandleEvents(frame_tp timepoint) {
    static frame_tp last_tp;
    this->delta = frame_unit(timepoint - last_tp);
    last_tp = timepoint;
    auto count = this->delta.count();
    for (auto& comp : this->computers) {
        comp.second.vc->Update(count);
    }
    for (auto& pbuffer : this->pixelBuffers) {
        tda::TDAScreen screen;
        pbuffer.second.second->DumpScreen(screen);
        tda::TDAtoRGBATexture(screen, (DWord*)pbuffer.second.first->LockWrite());
        pbuffer.second.first->UnlockWrite();
        pbuffer.second.first->Invalidate();
    }
}


bool VComputerSystem::LoadROMFile(const id_t entity_id, std::string fname) {
    if (this->computers.find(entity_id) != this->computers.end()) {
        auto& vc = this->computers[entity_id].vc;
        int size = LoadROM(fname, this->computers[entity_id].rom);
        if (size < 0) {
            std::fprintf(stderr, "An error hapen when was reading the file %s\n", fname.c_str());
            return false;
        }
        this->computers[entity_id].rom_size = size;
        vc->SetROM(this->computers[entity_id].rom, this->computers[entity_id].rom_size);
        return true;
    }
    return false;
}

void VComputerSystem::TurnComptuerOn(const id_t entity_id) {
    if (this->computers.find(entity_id) != this->computers.end()) {
        this->computers[entity_id].vc->On();
    }
}

void VComputerSystem::TurnComptuerOff(const id_t entity_id) {
    if (this->computers.find(entity_id) != this->computers.end()) {
        this->computers[entity_id].vc->Off();
    }
}

void VComputerSystem::AddComponent(const id_t entity_id, std::shared_ptr<ComponentBase> component) { }

void VComputerSystem::Notify(const KeyboardEvent* key_event) {
    switch (key_event->action) {
    case KeyboardEvent::KEY_DOWN:
        this->gkeyb->SendKeyEvent(key_event->scancode, key_event->key, gkeyboard::KEY_MODS::KEY_MOD_NONE);
    default:
        break;
    }
}

} // namespace trillek
