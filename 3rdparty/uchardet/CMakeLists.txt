add_library(uchardet STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/CharDistribution.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/JpCntx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangArabicModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangBulgarianModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangRussianModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangEsperantoModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangFrenchModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangDanishModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangGermanModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangGreekModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangHungarianModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangHebrewModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangSpanishModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangThaiModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangTurkishModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/LangModels/LangVietnameseModel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsHebrewProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsCharSetProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsBig5Prober.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsEUCJPProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsEUCKRProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsEUCTWProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsEscCharsetProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsEscSM.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsGB2312Prober.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsMBCSGroupProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsMBCSSM.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsSBCSGroupProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsSBCharSetProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsSJISProber.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsUTF8Prober.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsLatin1Prober.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nsUniversalDetector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/uchardet.cpp
)

target_include_directories(uchardet SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(uchardet SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/uchardet")