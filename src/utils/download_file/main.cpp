#include <stdio.h>

#include <do_config.h>
#include <do_download.h>

int main(int argc, char** argv)
{
    const std::error_code doErrorCode = microsoft::deliveryoptimization::download::download_url_to_path(
        argv[1], argv[2], false, std::chrono::seconds(30));
    if (!doErrorCode)
    {
        printf("Failed to download: %d", doErrorCode.value());
        return -1;
    }

    printf("Download Succeeded");
    return 0;
}
