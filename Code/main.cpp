#include "Structures/array.hpp"
#include "Structures/view.hpp"

struct Nom1
{
    int a;
};
struct Nom2
{
    int a;
};
struct Nom3
{
    int a;
};

Int32 main()
{
    Array<View<Nom1, Nom2, Nom3>, 10> aa{ { {10}, {11}, {12} } };

    for (auto &[n1, n2, n3] : aa)
    {
        SPDLOG_INFO("n1 {}, n2 {}, n3 {}", n1.a, n2.a, n3.a);
    }

    return 0;
}
