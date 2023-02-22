#include <mast1c0re.hpp>
#include "elf/elf.hpp"

#define LISTEN_PORT 9045

#define MAGIC 0x0000EA6E
#define DOWNLOAD_CHUNK_SIZE 4096
#define DOWNLOAD_BAR_UPDATE_FREQUENCY 2500

#define ELF_BUFFER  0x1000000

typedef void fEntry();
void setProgress(PS::Sce::MsgDialogProgressBar dialog, size_t downloaded, size_t total);

void main()
{
    // PS2 Breakout
    PS::Breakout::init();

    // Connect to debug server
    // PS::Debug.connect(IP(192, 168, 0, 7), 9017);

    // ELF entry point
    fEntry* entry = nullptr;

    // Create TCP server
    PS::TcpServer server = PS::TcpServer();
    if (server.listen(LISTEN_PORT))
    {
        // Show progress bar dialog
        PS::Sce::MsgDialogProgressBar dialog = PS::Sce::MsgDialogProgressBar("Waiting for PS2 ELF payload...");
        dialog.open();

        // Accept connection
        PS::Debug.printf("Waiting for client connection...\n");
        PS::TcpClient client = server.accept();

        // Get magic
        PS::Debug.printf("Waiting for magic...\n");
        if (client.read<uint32_t>() == MAGIC)
        {
            // Get filesize
            PS::Debug.printf("Waiting for data size...\n");
            size_t filesize = client.read<size_t>();
            PS::Debug.printf("Download file of size: %llu\n", filesize);

            setProgress(dialog, 0, filesize);

            // Download ELF from network
            PS::Debug.printf("Reading PS2 ELF from client...\n");

            uint8_t* buffer = (uint8_t*)ELF_BUFFER;
            uint32_t offset = 0;
            uint32_t updateBar = 0;
            for (uint64_t i = 0; i < filesize; i+= DOWNLOAD_CHUNK_SIZE)
            {
                uint32_t readSize = DOWNLOAD_CHUNK_SIZE;

                // Set size for final read if we are at the end
                if (i + readSize > filesize)
                    readSize = filesize % DOWNLOAD_CHUNK_SIZE;

                // Read into buffer
                client.read(buffer + offset, readSize);
                offset += readSize;

                // Update progress bar
                if (updateBar == DOWNLOAD_BAR_UPDATE_FREQUENCY)
                {
                    setProgress(dialog, i, filesize);
                    updateBar = 0;
                }
                updateBar++;
            }

            // Validate file size
            if (offset == filesize)
            {
                // Parse ELF
                Elf* elf = (Elf*)buffer;

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
                PS::Debug.printf("Received a file which resulted in a different size to what was expected. %llu was received, however we expected to eeceive %llu\n", offset, filesize);
                PS::notification("Error: Failed to download file");
            }
        }
        else
        {
            PS::Debug.printf("Invalid network magic received\n");
            PS::notification("Error: Invalid magic value received");
        }

        // Disconnect
        PS::Debug.printf("Closing connection...\n");
        client.disconnect();
        server.disconnect();
        dialog.terminate();
    }
    else
        PS::notification("Failed to listen on port %i!", LISTEN_PORT);

    // Disconnect from debug server
    PS::Debug.disconnect();

    // Execute loaded ELF
    if (entry != nullptr)
        entry();

    // Restore corruption
    PS::Breakout::restore();
}

void setProgress(PS::Sce::MsgDialogProgressBar dialog, size_t downloaded, size_t total)
{
    dialog.setMsg("Downloading PS2 ELF...\n");

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