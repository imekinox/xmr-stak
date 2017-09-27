
#pragma once

#include "autoAdjust.hpp"

#include "nvcc_code/cryptonight.h"
#include "jconf.h"
#include "../../console.h"
#include "../../ConfigEditor.hpp"

#include <vector>
#include <cstdio>
#include <sstream>
#include <string>


namespace xmrstak
{
namespace nvidia
{

class autoAdjust
{    
public:

    autoAdjust()
    {

    }

    /** print the adjusted values if needed
     *
     * Routine exit the application and print the adjusted values if needed else
     * nothing is happened.
     */
    bool printConfig()
    {
		int deviceCount = 0;
        if(cuda_get_devicecount(&deviceCount) == 0)
            std::exit(0);
        // evaluate config parameter for if auto adjustment is needed
                // evaluate config parameter for if auto adjustment is needed
        for(int i = 0; i < deviceCount; i++)
        {
       
            nvid_ctx ctx;

            ctx.device_id = i;
            // -1 trigger auto adjustment
            ctx.device_blocks = -1;
            ctx.device_threads = -1;

        // set all evice option those marked as auto (-1) to a valid value
#ifndef _WIN32
            ctx.device_bfactor = 0;
            ctx.device_bsleep = 0;
#else
            // windows pass, try to avoid that windows kills the miner if the gpu is blocked for 2 seconds
            ctx.device_bfactor = 6;
            ctx.device_bsleep = 25;
#endif
            if( cuda_get_deviceinfo(&ctx) != 1 )
            {
                printer::inst()->print_msg(L0, "Setup failed for GPU %d. Exitting.\n", i);
                std::exit(0);
            }
            nvidCtxVec.push_back(ctx);

        }

        generateThreadConfig();
		return true;

    }

private:
    
    void generateThreadConfig()
    {
		// load the template of the backend config into a char variable
		const char *tpl =
			#include "./config.tpl"
		;

		ConfigEditor configTpl{};
		configTpl.set( std::string(tpl) );

		constexpr size_t byte2mib = 1024u * 1024u;
		std::string conf;
        int i = 0;
        for(auto& ctx : nvidCtxVec)
        {
			conf += std::string("  // gpu: ") + ctx.name + " architecture: " + std::to_string(ctx.device_arch[0] * 10 + ctx.device_arch[1]) + "\n";
			conf += std::string("  //      memory: ") + std::to_string(ctx.free_device_memory / byte2mib) + "/"  + std::to_string(ctx.total_device_memory / byte2mib) + " MiB\n";
            conf += std::string("  { \"index\" : ") + std::to_string(ctx.device_id) + ",\n" +
                "    \"threads\" : " + std::to_string(ctx.device_threads) + ", \"blocks\" : " + std::to_string(ctx.device_blocks) + ",\n" +
                "    \"bfactor\" : " + std::to_string(ctx.device_bfactor) + ", \"bsleep\" :  " + std::to_string(ctx.device_bsleep) + ",\n" +
                "    \"affine_to_cpu\" : false,\n" +
                "  },\n";
            ++i;
        }

		configTpl.replace("GPUCONFIG",conf);
		configTpl.write("nvidia.txt");
		printer::inst()->print_msg(L0, "NVIDIA: GPU configuration stored in file '%s'", "nvidia.txt");
    }

    std::vector<nvid_ctx> nvidCtxVec;
};

} // namespace nvidia
} // namepsace xmrstak
