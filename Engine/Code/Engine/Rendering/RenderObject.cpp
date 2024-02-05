#include "RenderObject.h"

const PSOGraphics* RenderObject::GetPSO(DrawCallType type)
{
    auto payloadIt = drawCallsPSOs.find(type);
    if (payloadIt != drawCallsPSOs.end())
    {
        return payloadIt->second;
    }
    else
    {
        return nullptr;
    }
}

RenderObjectPtr RenderObject::Create()
{
    return std::make_shared<RenderObject>();
}