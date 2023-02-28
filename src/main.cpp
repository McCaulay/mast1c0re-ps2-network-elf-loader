#include <mast1c0re.hpp>
#include "elf/elf.hpp"

#define LISTEN_PORT 9045

#define MAGIC 0x0000EA6E
#define ELF_MAGIC 0x464c457f // \x7fELF
#define DOWNLOAD_CHUNK_SIZE 4096
#define DOWNLOAD_BAR_UPDATE_FREQUENCY 2500

#define ELF_BUFFER  0x1000000

typedef void fEntry();
bool downloadAndExecuteELF();
void setProgress(PS::Sce::MsgDialogProgressBar dialog, size_t downloaded, size_t total);

void main()
{
    // PS2 Breakout
    PS::Breakout::init();

    // Connect to debug server
    // PS::Debug.connect(IP(192, 168, 0, 7), 9017);

    // Keep executing PS2 ELF files
    while (downloadAndExecuteELF());

    // Disconnect from debug server
    PS::Debug.disconnect();

    // Restore corruption
    PS::Breakout::restore();
}

bool downloadAndExecuteELF()
{
    // ELF entry point
    fEntry* entry = nullptr;

    // Create TCP server
    PS::TcpServer server = PS::TcpServer();
    if (!server.listen(LISTEN_PORT))
    {
        PS::notification("Failed to listen on port %i!", LISTEN_PORT);
        return false;
    }

    // Show progress bar dialog
    PS::Sce::MsgDialog::Initialize();
    PS::Sce::MsgDialogProgressBar dialog = PS::Sce::MsgDialogProgressBar("Waiting for PS2 ELF payload...");
    dialog.open();
    dialog.setValue(0);

    // Accept connection
    PS::Debug.printf("Waiting for client connection...\n");
    PS::TcpClient client = server.accept();

    dialog.setMsg("Downloading PS2 ELF...");

    // Download variables
    uint8_t* buffer = (uint8_t*)ELF_BUFFER;
    size_t offset = 0;
    size_t filesize = 0;
    size_t headerSize = 0;
    size_t fileStart = 0;

    // Check if file is sent with filesize
    client.read(buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    if (*(uint32_t*)(buffer) == MAGIC)
    {
        // Read filesize
        client.read(buffer + offset, sizeof(size_t));
        filesize = *(size_t*)(buffer + offset);
        offset += sizeof(size_t);
        PS::Debug.printf("Download file of size: %llu\n", filesize);

        // Update dialog
        setProgress(dialog, 0, filesize);
        headerSize = sizeof(uint32_t) + sizeof(size_t);

        fileStart = headerSize;
    }
    else
        PS::Debug.printf("Download file of unknown size\n");

    // Download ELF from network
    PS::Debug.printf("Reading PS2 ELF from client...\n");

    // Read next 4 bytes
    size_t readCount = client.read(buffer + offset, sizeof(uint32_t));
    offset += readCount;

    // Early ELF magic validation
    if (*(uint32_t*)(buffer + fileStart) == ELF_MAGIC)
    {
        uint32_t updateBar = 0;
        while (true)
        {
            // Read into buffer
            readCount = client.read(buffer + offset, DOWNLOAD_CHUNK_SIZE);
            offset += readCount;

            if (readCount != DOWNLOAD_CHUNK_SIZE)
                break;

            // Update progress bar
            if (updateBar == DOWNLOAD_BAR_UPDATE_FREQUENCY)
            {
                if (filesize > 0)
                    setProgress(dialog, offset - headerSize, filesize);
                updateBar = 0;
            }
            updateBar++;
        }

        // Validate file size
        if (filesize == 0 || (offset - headerSize) == filesize)
        {
            // Parse ELF
            Elf* elf = (Elf*)(buffer + headerSize);

            // Validate header
            const char* error = elf->header.validate();
            if (error == nullptr)
            {
                // Load program sections into memory
                PS::Debug.printf("Enumerating program table...\n");
                ElfProgram* programs = elf->getProgramTable();
                for (uint32_t i = 0; i < elf->header.e_phnum; i++)
                {
                    // Ignore non-loadable program sections
                    if (programs[i].p_type != PT_LOAD)
                        continue;

                    uint8_t* section = (uint8_t*)((uint32_t)elf + programs[i].p_offset);

                    // Copy section from ELF section to given destination address
                    PS::Debug.printf("Loading 0x%06x to 0x%06x (0x%06x)\n",
                        section,
                        programs[i].p_vaddr,
                        programs[i].p_filesz
                    );
                    PS2::memcpy(programs[i].p_vaddr, section, programs[i].p_filesz);
                    if (programs[i].p_memsz > programs[i].p_filesz)
                        PS2::memset((void*)((uint32_t)programs[i].p_vaddr + programs[i].p_filesz), 0, programs[i].p_memsz - programs[i].p_filesz);
                }

                // Set entry point
                PS::Debug.printf("Entry: 0x%08x\n", elf->header.e_entry);
                entry = (fEntry*)elf->header.e_entry;
            }
            else
            {
                PS::Debug.printf("Header validation error: %s\n", error);
                PS::notification("Error: %s", error);
            }
        }
        else
        {
            PS::Debug.printf("Error: Received a file which resulted in a different size to what was expected. %llu was received, however we expected to eeceive %llu\n", offset - headerSize, filesize);
            PS::notification("Error: Failed to download file");
        }
    }
    else
    {
        PS::Debug.printf("Error: Invalid ELF magic");
        PS::notification("Error: Invalid ELF magic");
    }

    // Disconnect
    PS::Debug.printf("Closing connection...\n");
    client.disconnect();

    dialog.setValue(100);
    dialog.close();
    PS::Sce::MsgDialog::Terminate();

    // Stop the server
    server.disconnect();

    // Execute loaded ELF
    if (entry != nullptr)
        entry();

    return true;
}

void setProgress(PS::Sce::MsgDialogProgressBar dialog, size_t downloaded, size_t total)
{
    // Calculate percentage without float/double
    uint64_t divident = downloaded * 100;
    uint64_t percentage = 0;

    while (divident >= total)
    {
        divident -= total;
        percentage++;
    }

    dialog.setValue(percentage);
}