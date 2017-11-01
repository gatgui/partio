#include <Partio.h>
#include <PartioSe.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>

void usage()
{
    std::cerr << "SYNOPSYS" << std::endl;
    std::cerr << "  testse [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS" << std::endl;
    std::cout << "  -i <path>  : Input file                           [required]" << std::endl;
    std::cout << "  -e <expr>  : Expression to run.                   [required]" << std::endl;
    std::cout << "  -n <name > : Name of attribute to bind result to. [required]" << std::endl;
    std::cout << "  -o <path>  : Output file                          [optional: ./testse.gto]" << std::endl;
    std::cout << "  -t <time>  : Time value                           [optional: 1]" << std::endl;
    std::cout << "  -v         : Verbose output                       [optional]" << std::endl;
    std::cout << "  -h         : Show this help                       [optional]" << std::endl;
    
    
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
#ifdef PARTIO_SE_ENABLED

    const char *inPath = 0;
    const char *expr = 0;
    const char *outPath = 0;
    const char *outAttrName = 0;
    double t = 1.0;
    bool verbose = false;

    int i = 1;
    while (i < argc)
    {
        if (!strcmp(argv[i], "-i"))
        {
            if (++i >= argc)
            {
                std::cerr << "Expected value after -i flag" << std::endl;
                return 1;
            }
            inPath = argv[i];
        }
        else if (!strcmp(argv[i], "-e"))
        {
            if (++i >= argc)
            {
                std::cerr << "Expected value after -e flag" << std::endl;
                return 1;
            }
            expr = argv[i];
        }
        else if (!strcmp(argv[i], "-o"))
        {
            if (++i >= argc)
            {
                std::cerr << "Expected value after -o flag" << std::endl;
                return 1;
            }
            outPath = argv[i];
        }
        else if (!strcmp(argv[i], "-t"))
        {
            if (++i >= argc)
            {
                std::cerr << "Expected value after -t flag" << std::endl;
                return 1;
            }
            if (sscanf(argv[i], "%lf", &t) != 1)
            {
                std::cerr << "Expected a numeric value after -t flag" << std::endl;
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-h"))
        {
            usage();
            return 0;
        }
        else if (!strcmp(argv[i], "-n"))
        {
            if (++i >= argc)
            {
                std::cerr << "Expected value after -n flag" << std::endl;
                return 1;
            }
            outAttrName = argv[i];
        }
        else if (!strcmp(argv[i], "-v"))
        {
            verbose = true;
        }
        ++i;
    }

    if (!inPath || !expr || !outAttrName)
    {
        usage();
        return 1;
    }

    if (!outPath)
    {
        outPath = "./testse.gto";
    }

    if (verbose)
    {
        std::cout << "== Read particles data from \"" << inPath << "\"..." << std::endl;
    }
    Partio::ParticlesDataMutable *p = Partio::read(inPath);
    if (!p)
    {
        std::cerr << "Failed to read particle data from \"" << inPath << "\"" << std::endl;
        return 1;
    }

    if (verbose)
    {
        std::cout << "== Create expression object \"" <<  expr << "\"..." << std::endl;
    }
    Partio::PartioSe pse(p, expr);
    if (!pse.isValid())
    {
        const std::vector<SeExpr2::Expression::Error> &errors = pse.getErrors();
        for (size_t i=0; i<errors.size(); ++i)
        {
            std::cerr << "[ERROR] " << errors[i].startPos << " - " << errors[i].endPos << " : " << errors[i].error << std::endl;
        }
        return 1;
    }

    if (verbose)
    {
        std::cout << "== Set evaluation time to " << t << std::endl;
    }
    pse.setTime(t);

    if (verbose)
    {
        std::cout << "== Bind expression to output attribute \"" << outAttrName << "\"" << std::endl;
    }
    if (!pse.bindOutput(outAttrName))
    {
        std::cerr << "Failed to bind expression output to particle attribute \"" << outAttrName << "\"" << std::endl;
        p->release();
        return 1;
    }

    if (verbose)
    {
        std::cout << "== Run expression..." << std::endl;
    }
    pse.runAll();

    if (verbose)
    {
        std::cout << "== Write particles to \"" << outPath << "\"..." << std::endl;
    }
    Partio::write(outPath, *p);

    p->release();

#endif

    return 0;
}
