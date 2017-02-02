#include <Partio.h>
#include <PartioSe.h>
#include <iostream>

int main(int argc, char **argv)
{
#ifdef PARTIO_USE_SEEXPR

    if (argc != 2)
    {
        std::cerr << "Usage: testse <path> <expr>" << std::endl;
        return 1;
    }

    Partio::ParticlesDataMutable *p = Partio::read(argv[1]);
    if (!p)
    {
        return 1;
    }

    Partio::PartioSe pse(p, argv[2]);

    pse.setTime(1.0);
    pse.runAll();
    
    Partio::write("testse.gto", *p);

    p->release();

#endif

    return 0;
}
