// Fill out your copyright notice in the Description page of Project Settings.


#include "PolygonTriangulator.h"
#include "ThirdParty/Earcut/earcut.hpp"
#include <array>

TArray<int32> UPolygonTriangulator::TriangulateWithEarcut(const TArray<FVector2D>& Outer, const TArray<FVector2D>& Inner)
{
    using Point = std::array<float, 2>;
    using Ring = std::vector<Point>;
    using Polygon = std::vector<Ring>;

    Ring OuterRing, InnerRing;

    for (const auto& Pt : Outer)
        OuterRing.push_back(std::array<float, 2>{ static_cast<float>(Pt.X), static_cast<float>(Pt.Y) });


    for (const auto& Pt : Inner)
        InnerRing.push_back(std::array<float, 2>{ static_cast<float>(Pt.X), static_cast<float>(Pt.Y) });


    Polygon poly;
    poly.push_back(OuterRing); // main loop
    if (Inner.Num() > 2)
        poly.push_back(InnerRing); // hole

    std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(poly);
    TArray<int32> Result;
    for (uint32_t index : indices)
        Result.Add(static_cast<int32>(index));

    return Result;
}