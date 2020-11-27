







#include <string>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "tcl.h"
#include "rcsTvfCompiler.h"

#include "rcstvfpackageprovide.hpp"

bool rcsTVFCompiler_T::m_isInRulecheck    = false;
char* rcsTVFCompiler_T::m_pTvfArg         = NULL;
char* rcsTVFCompiler_T::m_pTvfFilename    = const_cast<char*>("");
char* rcsTVFCompiler_T::m_pSvrfFileName   = const_cast<char*>("");
Tcl_Interp* rcsTVFCompiler_T::m_pInterp   = NULL;

std::ofstream rcsTVFCompiler_T::m_fSvrf;
std::map<std::string, std::string, rcsTVFCompiler_T::LTstr> rcsTVFCompiler_T::layermap_g;
std::map<std::string, std::string, rcsTVFCompiler_T::LTstr> rcsTVFCompiler_T::layermap_l;
std::map<std::string, std::string, rcsTVFCompiler_T::LTstr> rcsTVFCompiler_T::varmap_g;

int
rcsTVFCompiler_T::get_global_var_proc(ClientData d, Tcl_Interp *pInterp,
                                      int argc, const char * argv[])
{
    if(argc < 2)
        return TCL_ERROR;

    std::string sVariableName;
    for(int i = 1; i < argc; ++i)
    {
        std::string sValue = argv[i];
        trim(sValue, "(),");
        if(sValue.empty())
            continue;

        sVariableName += sValue;
        sVariableName += " ";
    }

    if(varmap_g.find(sVariableName) != varmap_g.end())
    {
        Tcl_SetResult(pInterp, const_cast<char*>(varmap_g[sVariableName].c_str()), TCL_VOLATILE);
    }
    else
    {
        Tcl_SetResult(pInterp, NULL, TCL_STATIC);
    }
    return TCL_OK;
}

int
rcsTVFCompiler_T::set_global_var_proc(ClientData d, Tcl_Interp *pInterp,
                                      int argc, const char * argv[])
{
    if(argc < 3)
        return TCL_ERROR;

    std::string sVariableName;
    for(int i = 1; i < argc - 1; ++i)
    {
        std::string sValue = argv[i];
        trim(sValue, "(),");
        if(sValue.empty())
            continue;

        sVariableName += sValue;
        sVariableName += " ";
    }

    pInterp->result = const_cast<char*>(argv[argc - 1]);
    varmap_g[sVariableName] = std::string(argv[argc - 1]);
    return TCL_OK;
}

int
rcsTVFCompiler_T::unset_global_var_proc(ClientData d, Tcl_Interp *pInterp,
                                        int argc, const char * argv[])
{
    if(argc < 2)
        return TCL_ERROR;

    std::string sVariableName;
    for(int i = 1; i < argc; ++i)
    {
        std::string sValue = argv[i];
        trim(sValue, "(),");
        if(sValue.empty())
            continue;

        sVariableName += sValue;
        sVariableName += " ";
    }

    if(varmap_g.find(sVariableName) != varmap_g.end())
        varmap_g.erase(sVariableName);

    return TCL_OK;
}

int
rcsTVFCompiler_T::exists_global_var_proc(ClientData d, Tcl_Interp *pInterp,
                                         int argc, const char * argv[])
{
    if(argc < 2)
        return TCL_ERROR;

    std::string sVariableName;
    for(int i = 1; i < argc; ++i)
    {
        std::string sValue = argv[i];
        trim(sValue, "(),");
        if(sValue.empty())
            continue;

        sVariableName += sValue;
        sVariableName += " ";
    }

    if(varmap_g.find(sVariableName) != varmap_g.end())
        Tcl_SetResult(pInterp, "1\0", TCL_STATIC);
    else
        Tcl_SetResult(pInterp, "0\0", TCL_STATIC);

    return TCL_OK;
}


int
rcsTVFCompiler_T::blockcomment_proc(ClientData d, Tcl_Interp *interp,
                                    int argc, const char * argv[])
{
    m_fSvrf << "// {" ;
    for(int i=1; i< argc; ++i)
    {
        if( i != 1 )
            m_fSvrf << " ";
        m_fSvrf << argv[i];
    }
    m_fSvrf << "}" << std::endl;
    return TCL_OK;
}


int
rcsTVFCompiler_T::linecomment_proc(ClientData d, Tcl_Interp *interp,
                                   int argc, const char * argv[])
{
    m_fSvrf << "@";
    for( int i = 1; i < argc; ++i )
    {
        m_fSvrf << " " << argv[i];
    }
    m_fSvrf << "\n";
    return TCL_OK;
}



int
rcsTVFCompiler_T::print_proc(ClientData d, Tcl_Interp *interp,
                             int argc, const char * argv[])
{
    if(m_isInRulecheck )
    {
        m_fSvrf << "\t";
    }

    for( int i=1; i<argc; ++i)
    {
        if( i == 1 )
            m_fSvrf << argv[i];
        else
            m_fSvrf << " " << argv[i];
    }
    m_fSvrf << std::endl;
    return TCL_OK;
}


int
rcsTVFCompiler_T::echo_to_svrf_file_proc(ClientData d, Tcl_Interp *interp,
                             int argc, const char * argv[])
{
    if( argc != 2 )
    {
        return TCL_ERROR;
    }

    if(m_isInRulecheck )
    {
        m_fSvrf << "\t";
    }

    for( int i=1; i<argc; ++i)
    {
        if( i == 1 )
            m_fSvrf << argv[i];
        else
            m_fSvrf << " " << argv[i];
    }
    m_fSvrf << std::endl;
    return TCL_OK;
}

int rcsTVFCompiler_T::output_to_svrf_proc(ClientData d, Tcl_Interp *interp, int argc, const char *argv[])
{
    
    return echo_to_svrf_file_proc(d, interp, argc, argv);
}




int
rcsTVFCompiler_T::svrfcmd_proc(ClientData d, Tcl_Interp *interp,
                               int argc, const char * argv[])
{
    if( m_isInRulecheck ) m_fSvrf << "\t";
    m_fSvrf << static_cast<char*>(d);
    for( int i=1; i<argc; ++i)
    {
        m_fSvrf << " " << argv[i];
    }
    m_fSvrf << std::endl;
    return TCL_OK;
}



int
rcsTVFCompiler_T::verbatim_proc(ClientData d, Tcl_Interp *interp,
                                int argc, const char * argv[])
{
    for( int i=1; i<argc; ++i)
    {
        if( i != 1 ) 
        	m_fSvrf << " ";
        m_fSvrf << argv[i];
    }
    m_fSvrf << std::endl;
    return TCL_OK;
}



int
rcsTVFCompiler_T::tvf_proc(ClientData d, Tcl_Interp *interp,
                           int argc, const char * argv[])
{
    m_fSvrf << "TVF ";
    for( int i=1; i<argc; ++i)
    {
        if( i != 1 ) 
        	m_fSvrf << " ";
        m_fSvrf << argv[i];
    }
    m_fSvrf << std::endl;
    return TCL_OK;
}



int
rcsTVFCompiler_T::gettvfarg_proc(ClientData d, Tcl_Interp *interp,
                                 int argc, const char * argv[])
{
    Tcl_SetResult( interp, m_pTvfArg, NULL );
    return TCL_OK;
}


int
rcsTVFCompiler_T::rulecheck_proc(ClientData d, Tcl_Interp *interp,
                                 int argc, const char * argv[])
{
    m_isInRulecheck = true;

    if(argc < 3)
    {
        printf("ERROR: less arguments for RULECHECK:\n");
        for(int i=0; i<argc;++i)
        {
            printf("%s ",argv[i] );
        }
        return TCL_ERROR;
    }
    std::string str = "";
    for(int i = 2; i < argc; ++i)
    {
        str += argv[i];
    }

    m_fSvrf << argv[1] << " {";
    int iRet = Tcl_RecordAndEval( interp, (char*)(str.c_str()), 0);
    if(iRet == TCL_ERROR)
    {
        printf("Error when compile rulecheck\n");
        return TCL_ERROR;
    }
    m_fSvrf << "}" << std::endl;

    m_isInRulecheck = false;
    return TCL_OK;
}

void
rcsTVFCompiler_T::freeResult(char *pValue)
{
    ::free(pValue);
}

static inline int
convertEnclosureMeasurementsToDrcCmds(Tcl_Interp *interp, int argc, const char * argv[],
                                      std::string &sValue, std::ofstream &fSvrf)
{
    static hvUInt32 iLayer = 1;
    static std::string sBreak = "__";
    std::string sBaseLayer;
    std::string sMeasLayer;
    std::string sOrientLayer;
    std::string sMaxPar;
    std::string sMaxPer;

    bool        isConnect = false;
    for(int iPara = 1; iPara < argc; ++iPara)
    {
        if(strcmp(argv[iPara], "-base") == 0)
        {
            if(!sBaseLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sBaseLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-measurement") == 0)
        {
            if(!sMeasLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMeasLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-orient") == 0)
        {
            if(!sOrientLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sOrientLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_par") == 0)
        {
            if(!sMaxPar.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxPar = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_per") == 0)
        {
            if(!sMaxPer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxPer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-connect") == 0)
        {
            isConnect = true;
        }
        else if(strcmp(argv[iPara], "-help") == 0 || strcmp(argv[iPara], "-?") == 0)
        {
            return TCL_ERROR;
        }
    }

    if(sMaxPar.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_par", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxPar = pValue;
        else
            return TCL_ERROR;
    }

    if(sMaxPer.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_per", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxPer = pValue;
        else
            return TCL_ERROR;
    }

    if(sMaxPer.empty() || sMaxPar.empty() || sBaseLayer.empty() ||
       sMeasLayer.empty() || sOrientLayer.empty())
        return TCL_ERROR;

    double fMaxPer;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxPer.c_str(), &fMaxPer))
        return TCL_ERROR;

    double fMaxPar;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxPar.c_str(), &fMaxPar))
        return TCL_ERROR;

    std::string sTmpLayerPrefix;

    sTmpLayerPrefix += sBaseLayer;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += sMeasLayer;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += "enc__TVF_tmp_";
    sTmpLayerPrefix += itoa(iLayer);

    std::string side_orient = sTmpLayerPrefix;
    side_orient += ".side_orient";

    fSvrf << side_orient << " = COINCIDENT EDGE " << sBaseLayer << " " << sOrientLayer << "\n";

    std::string end_orient = sTmpLayerPrefix;
    end_orient += ".end_orient";

    fSvrf << end_orient << " = NOT COINCIDENT EDGE " << sBaseLayer << " " << sOrientLayer << "\n";

    std::string side_enclosure_dfmspace = sTmpLayerPrefix;
    side_enclosure_dfmspace += ".side_enclosure.dfmspace";

    fSvrf << side_enclosure_dfmspace << " = DFM SPACE " << side_orient << " " << sMeasLayer
          << " <= " << sMaxPer << " BY ENC BY LAYER " << sMeasLayer << " COUNT == 0" << "\n";

    std::string end_enclosure_dfmspace = sTmpLayerPrefix;
    end_enclosure_dfmspace += ".end_enclosure.dfmspace";

    fSvrf << end_enclosure_dfmspace << " = DFM SPACE " << end_orient << " " << sMeasLayer
          << " <= " << sMaxPar << " BY ENC BY LAYER " << sMeasLayer << " COUNT == 0" << "\n";

    if(!isConnect)
    {
        std::string side = sTmpLayerPrefix;
        side += ".side";

        fSvrf << side << " = DFM PROPERTY " << side_orient << " " << side_enclosure_dfmspace
              << " OVERLAP ABUT ALSO MULTI\n [PER = RANGE_XXY(SORT_MERGE_XXY(CONCAT(VECTOR(ECMIN("
              << side_enclosure_dfmspace << "), ECMAX(" << side_enclosure_dfmspace << "), EW("
              << side_enclosure_dfmspace << ")), VECTOR(ECMIN(" << side_orient << "), ECMAX(" << side_orient
              << "), " << sMaxPer << "))), VECTOR(ECMIN(" << side_orient << "), ECMAX(" << side_orient << ")))]" << "\n\n";

        std::string end = sTmpLayerPrefix;
        end += ".end";

        fSvrf << end << " = DFM PROPERTY " << end_orient << " " << end_enclosure_dfmspace
              << " OVERLAP ABUT ALSO MULTI\n [PAR = RANGE_XXY(SORT_MERGE_XXY(CONCAT(VECTOR(ECMIN("
              << end_enclosure_dfmspace << "), ECMAX(" << end_enclosure_dfmspace << "), EW(" << end_enclosure_dfmspace
              << ")), VECTOR(ECMIN(" << end_orient << "), ECMAX(" << end_orient << "), " << sMaxPar << "))), VECTOR(ECMIN("
              << end_orient << "), ECMAX(" << end_orient << ")))]" << "\n\n";

        sValue.clear();

        sValue += "DFM PROPERTY ";
        sValue += sBaseLayer;
        sValue += " ";
        sValue += side;
        sValue += " ";
        sValue += end;
        sValue += " OVERLAP ABUT ALSO\n \t[PER_1 = VPROPERTY(";
        sValue += side;
        sValue += ", PER, 1)]\n";
        sValue += "\t[PER_2 = VPROPERTY(";
        sValue += side;
        sValue += ", PER, 2)]\n";
        sValue += "\t[PAR_1 = VPROPERTY(";
        sValue += end;
        sValue += ", PAR, 1)]\n";
        sValue += "\t[PAR_2 = VPROPERTY(";
        sValue += end;
        sValue += ", PAR, 2)]\n";
    }
    else
    {
        std::string side_netid = sTmpLayerPrefix;
        side_netid += ".side.netid";

        fSvrf << side_netid << " = DFM PROPERTY " << sOrientLayer << " "
              << "[NETID = NETID(" << sOrientLayer << ")]\n";

        std::string side = sTmpLayerPrefix;
        side += ".side";

        fSvrf << side << " = DFM PROPERTY " << side_orient << " " << side_enclosure_dfmspace
              << " " << side_netid << " OVERLAP ABUT ALSO MULTI\n\t[PER = RANGE_XXY(SORT_MERGE_XXY(CONCAT(VECTOR(ECMIN("
              << side_enclosure_dfmspace << "), ECMAX(" << side_enclosure_dfmspace << "), EW(" << side_enclosure_dfmspace
              << ")), VECTOR(ECMIN(" << side_orient << "), ECMAX(" << side_orient << "), " << sMaxPer << "))), VECTOR(ECMIN("
              << side_orient << "), ECMAX(" << side_orient << ")))]" << "\n\t[NETID = NETPROPERTY(" << side_netid << ", \"NETID\", 1)]" << "\n\n";

        std::string end = sTmpLayerPrefix;
        end += ".end";

        fSvrf << end << " = DFM PROPERTY " << end_orient << " " << end_enclosure_dfmspace
              << " OVERLAP ABUT ALSO MULTI\n [PAR = RANGE_XXY(SORT_MERGE_XXY(CONCAT(VECTOR(ECMIN("
              << end_enclosure_dfmspace << "), ECMAX(" << end_enclosure_dfmspace << "), EW(" << end_enclosure_dfmspace
              << ")), VECTOR(ECMIN(" << end_orient << "), ECMAX(" << end_orient << "), " << sMaxPar << "))), VECTOR(ECMIN("
              << end_orient << "), ECMAX(" << end_orient << ")))]" << "\n\n";

        sValue.clear();

        sValue += "DFM PROPERTY ";
        sValue += sBaseLayer;
        sValue += " ";
        sValue += side;
        sValue += " ";
        sValue += end;
        sValue += " OVERLAP ABUT ALSO\n \t[PER_1 = VPROPERTY(";
        sValue += side;
        sValue += ", PER, 1)]\n";
        sValue += "\t[PER_2 = VPROPERTY(";
        sValue += side;
        sValue += ", PER, 2)]\n";
        sValue += "\t[PAR_1 = VPROPERTY(";
        sValue += end;
        sValue += ", PAR, 1)]\n";
        sValue += "\t[PAR_2 = VPROPERTY(";
        sValue += end;
        sValue += ", PAR, 2)]\n";
        sValue += "\t[PER_NETID_1 = NETPROPERTY(";
        sValue += side;
        sValue += ", NETID, 1) ]\n";
        sValue += "\t[PER_NETID_2 = NETPROPERTY(";
        sValue += side;
        sValue += ", NETID, 2) ]\n";
    }
    sValue += "\n";

    ++iLayer;

    return TCL_OK;
}

int
rcsTVFCompiler_T::enclosure_measurements_proc(ClientData d, Tcl_Interp *interp,
                                              int argc, const char * argv[])
{
    static std::string sMessage = "\nlvs : device::enclosure_measurements [options] \noptions:\n\
    -base value          Base layer (usually the gate layer) <>\n\
    -measurement value   Measurement layer (usually the well layer) <>\n\
    -orient value        Orientation layer (usually the src/drn layer) <>\n\
    -max_par value       Base layer to measurement layer maximum search distance override (PAR) <-1.0>\n\
    -max_per value       Base layer to measurement layer maximum search distance override (PER) <-1.0>\n\
    -connect             Store PER_NETID_1 and PER_NETID_2, -orient must be connected layer\n\
    -help                Print this message\n\
    -?                   Print this message\n\n";

    std::string sResult;
    if(TCL_ERROR == convertEnclosureMeasurementsToDrcCmds(interp, argc, argv, sResult, m_fSvrf))
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}

static inline int
convertStrainedSiliconMeasurementsToDrcCmds(Tcl_Interp *interp, int argc, const char * argv[],
                                            std::string &sValue, std::ofstream &fSvrf)
{
    static hvUInt32 iLayer = 1;
    static std::string sBreak = "__";
    std::string sGateLayer;
    std::string sSdLayer;
    std::string sActiveLayer;
    std::string sPolyLayer;
    std::string sLayer1;
    std::string sMaxSp;
    std::string sMaxSp2;
    std::string sMaxSd;
    std::string sMaxSe;
    std::string sMaxSf;
    std::string sMaxSode;
    std::string sMaxSodf;
    std::string sMaxSle;
    std::string sMaxSlf;
    std::string sMaxSodlf;
    std::string sLatSodlf;

    for(int iPara = 1; iPara < argc; ++iPara)
    {
        if(strcmp(argv[iPara], "-gate") == 0)
        {
            if(!sGateLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sGateLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-sd") == 0)
        {
            if(!sSdLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sSdLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-active") == 0)
        {
            if(!sActiveLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sActiveLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-poly") == 0)
        {
            if(!sPolyLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sPolyLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-layer1") == 0)
        {
            if(!sLayer1.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sLayer1 = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sp") == 0)
        {
            if(!sMaxSp.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSp = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sp2") == 0)
        {
            if(!sMaxSp2.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSp2 = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sd") == 0)
        {
            if(!sMaxSd.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSd = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_se") == 0)
        {
            if(!sMaxSe.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSe = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sf") == 0)
        {
            if(!sMaxSf.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSf = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sode") == 0)
        {
            if(!sMaxSode.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSode = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sodf") == 0)
        {
            if(!sMaxSodf.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSodf = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sle") == 0)
        {
            if(!sMaxSle.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSle = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_slf") == 0)
        {
            if(!sMaxSlf.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSlf = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_sodlf") == 0)
        {
            if(!sMaxSodlf.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxSodlf = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-lat_sodlf") == 0)
        {
            if(!sLatSodlf.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sLatSodlf = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-help") == 0 || strcmp(argv[iPara], "-?") == 0)
        {
            return TCL_ERROR;
        }
    }

    if(sMaxSp.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sp", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSp = pValue;
        else
            return TCL_ERROR;
    }

    if(sMaxSp2.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sp2", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSp2 = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSd.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sd", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSd = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSe.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_se", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSe = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSf.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sf", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSf = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSode.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sode", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSode = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSodf.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sodf", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSodf = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSle.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sle", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSle = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSlf.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_slf", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSlf = pValue;
        else
            return TCL_ERROR;
    }
    if(sMaxSodlf.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_sodlf", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxSodlf = pValue;
        else
            return TCL_ERROR;
    }
    if(sLatSodlf.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::lat_sodlf", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sLatSodlf = pValue;
        else
            return TCL_ERROR;
    }

    if(sMaxSp2.empty() || sMaxSp.empty() || sMaxSd.empty() || sMaxSf.empty() ||
       sMaxSode.empty() || sMaxSodf.empty()  || sMaxSle.empty() || sMaxSlf.empty() ||
       sLatSodlf.empty() || sMaxSodlf.empty() || sGateLayer.empty() || sSdLayer.empty() ||
       sActiveLayer.empty() || sPolyLayer.empty() || sLayer1.empty())
        return TCL_ERROR;

    double fMaxSp;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSp.c_str(), &fMaxSp))
        return TCL_ERROR;
    double fMaxSp2;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSp2.c_str(), &fMaxSp2))
        return TCL_ERROR;
    double fMaxSd;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSd.c_str(), &fMaxSd))
        return TCL_ERROR;
    double fMaxSe;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSe.c_str(), &fMaxSe))
        return TCL_ERROR;
    double fMaxSf;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSf.c_str(), &fMaxSf))
        return TCL_ERROR;
    double fMaxSode;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSode.c_str(), &fMaxSode))
        return TCL_ERROR;
    double fMaxSodf;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSodf.c_str(), &fMaxSodf))
        return TCL_ERROR;
    double fMaxSle;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSle.c_str(), &fMaxSle))
        return TCL_ERROR;
    double fMaxSlf;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSlf.c_str(), &fMaxSlf))
        return TCL_ERROR;
    double fMaxSodlf;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxSodlf.c_str(), &fMaxSodlf))
        return TCL_ERROR;
    double fLatSodlf;
    if(TCL_ERROR == Tcl_GetDouble(interp, sLatSodlf.c_str(), &fLatSodlf))
        return TCL_ERROR;

    std::string sTmpLayerPrefix;

    sTmpLayerPrefix += sGateLayer;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += sLayer1;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += "strsil__TVF_tmp_";
    sTmpLayerPrefix += itoa(iLayer);

    std::string side_orient = sTmpLayerPrefix;
    side_orient += ".side_orient";

    fSvrf << side_orient << " = COINCIDENT EDGE " << sGateLayer << " " << sSdLayer << std::endl;

    std::string end_orient = sTmpLayerPrefix;
    end_orient += ".end_orient";

    fSvrf << end_orient << " = NOT COINCIDENT EDGE " << sGateLayer << " " << sSdLayer << "\n";

    std::string g2slfod_lat_sodlf_prep = sTmpLayerPrefix;
    g2slfod_lat_sodlf_prep += ".g2slfod.lat_sodlf.prep";

    fSvrf << g2slfod_lat_sodlf_prep << " = " << sGateLayer << " OR (EXPAND EDGE "
          << side_orient << " BY " << sLatSodlf << ")\n";

    std::string g2slfod_lat_sodlf = sTmpLayerPrefix;
    g2slfod_lat_sodlf += ".g2slfod.lat_sodlf";

    fSvrf << g2slfod_lat_sodlf << " = TOUCH EDGE " << g2slfod_lat_sodlf_prep << " " << end_orient << "\n";

    std::string g2sp_dfmspace = sTmpLayerPrefix;
    g2sp_dfmspace += ".g2sp.dfmspace";

    fSvrf << g2sp_dfmspace << " = DFM SPACE " << side_orient << " " << sPolyLayer
          << " <= " << sMaxSp2 << " BY EXT BY LAYER " << sPolyLayer << " COUNT <= 2\n";

    std::string g2sd_dfmspace = sTmpLayerPrefix;
    g2sd_dfmspace += ".g2sd.dfmspace";

    fSvrf << g2sd_dfmspace << " = DFM SPACE " << side_orient << " " << sActiveLayer
          << " <= " << sMaxSd << " BY ENC BY LAYER " << sActiveLayer << " COUNT == 0\n";

    std::string g2se_dfmspace = sTmpLayerPrefix;
    g2se_dfmspace += ".g2se.dfmspace";

    fSvrf << g2se_dfmspace << " = DFM SPACE " << side_orient << " " << sLayer1
          << " <= " << sMaxSe << " BY ENC BY LAYER " << sLayer1 << " COUNT == 0\n";

    std::string g2sf_dfmspace = sTmpLayerPrefix;
    g2sf_dfmspace += ".g2sf.dfmspace";

    fSvrf << g2sf_dfmspace << " = DFM SPACE " << side_orient << " " << sLayer1
          << " <= " << sMaxSf << " BY EXT BY LAYER " << sLayer1 << " COUNT <= 1 MEASURE ALL\n";

    std::string g2seod_dfmspace = sTmpLayerPrefix;
    g2seod_dfmspace += ".g2seod.dfmspace";

    fSvrf << g2seod_dfmspace << " = DFM SPACE " << side_orient << " " << sActiveLayer
          << " <= " << sMaxSode << " BY ENC BY LAYER " << sActiveLayer << " COUNT == 0\n";

    std::string g2sfod_dfmspace = sTmpLayerPrefix;
    g2sfod_dfmspace += ".g2sfod.dfmspace";

    fSvrf << g2sfod_dfmspace << " = DFM SPACE " << side_orient << " " << sActiveLayer
          << " <= " << sMaxSodf << " BY EXT BY LAYER " << sActiveLayer << " COUNT <= 1 MEASURE ALL\n";

    std::string g2sle_dfmspace = sTmpLayerPrefix;
    g2sle_dfmspace += ".g2sle.dfmspace";

    fSvrf << g2sle_dfmspace << " = DFM SPACE " << end_orient << " " << sLayer1
          << " <= " << sMaxSle << " BY ENC BY LAYER " << sLayer1 << " COUNT == 0\n";

    std::string g2slf_dfmspace = sTmpLayerPrefix;
    g2slf_dfmspace += ".g2slf.dfmspace";

    fSvrf << g2slf_dfmspace << " = DFM SPACE " << end_orient << " " << sLayer1 << " <= "
          << sMaxSlf << " BY EXT BY LAYER " << sLayer1 << " COUNT <= 1 MEASURE ALL\n";

    std::string g2slfod_dfmspace = sTmpLayerPrefix;
    g2slfod_dfmspace += ".g2slfod.dfmspace";

    fSvrf << g2slfod_dfmspace << " = DFM SPACE " << g2slfod_lat_sodlf << " " << sActiveLayer
          << " <= " << sMaxSodlf << " BY EXT BY LAYER " << sActiveLayer << " COUNT <= 1 MEASURE ALL\n";

    std::string pse_combine = sTmpLayerPrefix;
    pse_combine += ".pse.combine";

    fSvrf << pse_combine << " = DFM PROPERTY " << side_orient << " " << g2sp_dfmspace << " "
          << g2sd_dfmspace << " OVERLAP ABUT ALSO MULTI \n [PSE_RANGE = VECTOR(ECMIN(" << side_orient
          << "), ECMAX(" << side_orient << "))]\n [PSE_POLY_SPACE_1 = CONCAT(VECTOR(ECMIN(" << g2sp_dfmspace
          << "), ECMAX(" << g2sp_dfmspace << "), EW(" << g2sp_dfmspace << ")), VECTOR(ECMIN(" << side_orient
          << "), ECMAX(" << side_orient << "), " << sMaxSp << "))]\n [PSE_POLY_SPACE_2 = CONCAT(VECTOR(ECMIN("
          << g2sp_dfmspace << "), ECMAX(" << g2sp_dfmspace << "), EW(" << g2sp_dfmspace << ")), VECTOR(ECMIN("
          << side_orient << "), ECMAX(" << side_orient << "), " << sMaxSp2 << "))]\n [PSE_DIFF_ENC = CONCAT(VECTOR(ECMIN("
          << g2sd_dfmspace << "), ECMAX(" << g2sd_dfmspace << "), EW(" << g2sd_dfmspace << ")), VECTOR(ECMIN("
          << side_orient << "), ECMAX(" << side_orient << "), " << sMaxSd << "))]\n\n";

    std::string pse_select = sTmpLayerPrefix;
    pse_select += ".pse.select";

    fSvrf << pse_select << " = DFM PROPERTY " << pse_combine << "\n [PSE = RANGE_XXY(SELECT_MERGE_XXY(1, VPROPERTY("
          << pse_combine << ", \"PSE_POLY_SPACE_1\"), 2, VPROPERTY(" << pse_combine << ", \"PSE_POLY_SPACE_2\"), 1, VPROPERTY("
          << pse_combine << ", \"PSE_DIFF_ENC\")), VPROPERTY(" << pse_combine << ", \"PSE_RANGE\"))]\n\n";

    std::string be_side_combine = sTmpLayerPrefix;
    be_side_combine += ".be_side.combine";

    fSvrf << be_side_combine << " = DFM PROPERTY " << side_orient << " " << g2sp_dfmspace
          << " " << g2se_dfmspace << " " << g2sf_dfmspace << " OVERLAP ABUT ALSO MULTI \n [BE_RANGE = VECTOR(ECMIN("
          << side_orient << "), ECMAX(" << side_orient << "))]\n [BE_POLY_SPACE = CONCAT(VECTOR(ECMIN("
          << g2sp_dfmspace << "), ECMAX(" << g2sp_dfmspace << "), EW(" << g2sp_dfmspace << ")), VECTOR(ECMIN("
          << side_orient << "), ECMAX(" << side_orient << "), " << sMaxSp << "))]\n [BE_LAYER1_ENC = CONCAT(VECTOR(ECMIN("
          << g2se_dfmspace << "), ECMAX(" << g2se_dfmspace << "), EW(" << g2se_dfmspace << ")), VECTOR(ECMIN(" << side_orient
          << "), ECMAX(" << side_orient << "), " << sMaxSe << "))]\n [BE_LAYER1_EXT = CONCAT(VECTOR(ECMIN(" << g2sf_dfmspace
          << "), ECMAX(" << g2sf_dfmspace << "), EW(" << g2sf_dfmspace << ")), VECTOR(ECMIN(" << side_orient << "), ECMAX("
          << side_orient << "), " << sMaxSf << "))]\n\n";

    std::string be_side_select = sTmpLayerPrefix;
    be_side_select += ".be_side.select";

    fSvrf << be_side_select << " = DFM PROPERTY " << be_side_combine << "\n [PSE = RANGE_XXY(SORT_MERGE_XXY(VPROPERTY("
          << be_side_combine << ", \"BE_POLY_SPACE\"), VPROPERTY(" << be_side_combine << ", \"BE_LAYER1_ENC\"), VPROPERTY("
          << be_side_combine << ", \"BE_LAYER1_EXT\")), VPROPERTY(" << be_side_combine << ", \"BE_RANGE\"))]\n\n";

    std::string lod_side_combine = sTmpLayerPrefix;
    lod_side_combine += ".lod_side.combine";

    fSvrf << lod_side_combine << " = DFM PROPERTY " << side_orient << " " << g2sp_dfmspace << " "
          << g2seod_dfmspace << " " << g2sfod_dfmspace << " OVERLAP ABUT ALSO MULTI \n [LOD_RANGE = VECTOR(ECMIN("
          << side_orient << "), ECMAX(" << side_orient << "))]\n [LOD_POLY_SPACE = CONCAT(VECTOR(ECMIN(" << g2sp_dfmspace
          << "), ECMAX(" << g2sp_dfmspace << "), EW(" << g2sp_dfmspace << ")), VECTOR(ECMIN(" << side_orient << "), ECMAX("
          << side_orient << "), " << sMaxSp << "))]\n [LOD_DIFF_ENC = CONCAT(VECTOR(ECMIN(" << g2seod_dfmspace << "), ECMAX("
          << g2seod_dfmspace << "), EW(" << g2seod_dfmspace << ")), VECTOR(ECMIN(" << side_orient << "), ECMAX(" << side_orient
          << "), " << sMaxSode << "))]\n [LOD_DIFF_EXT = CONCAT(VECTOR(ECMIN(" << g2sfod_dfmspace << "), ECMAX(" << g2sfod_dfmspace
          << "), EW(" << g2sfod_dfmspace << ")), VECTOR(ECMIN(" << side_orient << "), ECMAX(" << side_orient << "), " << sMaxSodf << "))]\n\n";

    std::string lod_side_select = sTmpLayerPrefix;
    lod_side_select += ".lod_side.select";

    fSvrf << lod_side_select << " = DFM PROPERTY " << lod_side_combine << "\n [PSE = RANGE_XXY(SORT_MERGE_XXY(VPROPERTY("
          << lod_side_combine << ", \"LOD_POLY_SPACE\"), VPROPERTY(" << lod_side_combine << ", \"LOD_DIFF_ENC\"), VPROPERTY("
          << lod_side_combine << ", \"LOD_DIFF_EXT\")), VPROPERTY(" << lod_side_combine << ", \"LOD_RANGE\"))]\n\n";

    std::string side = sTmpLayerPrefix;
    side += ".side";

    fSvrf << side << " = DFM PROPERTY " << side_orient << " " << pse_select << " " << be_side_select
          << " " << lod_side_select << " OVERLAP ABUT ALSO MULTI\n [PSE = VPROPERTY(" << pse_select
          << ", \"PSE\")]\n [BE = VPROPERTY(" << be_side_select << ", \"BE\")]\n [LOD = VPROPERTY("
          << lod_side_select << ", \"LOD\")]\n\n";

    std::string be_end_combine = sTmpLayerPrefix;
    be_end_combine += ".be_end.combine";

    fSvrf << be_end_combine << " = DFM PROPERTY " << end_orient << " " << g2sle_dfmspace
          << " " << g2slf_dfmspace << " OVERLAP ABUT ALSO MULTI \n [BE_RANGE = VECTOR(ECMIN("
          << end_orient << "), ECMAX(" << end_orient << "))]\n [BE_LAYER_1_ENC = CONCAT(VECTOR(ECMIN("
          << g2sle_dfmspace << "), ECMAX(" << g2sle_dfmspace << "), EW(" << g2sle_dfmspace << ")), VECTOR(ECMIN("
          << end_orient << "), ECMAX(" << end_orient << "), " << sMaxSle << "))]\n [BE_LAYER1_EXT = CONCAT(VECTOR(ECMIN("
          << g2slf_dfmspace << "), ECMAX(" << g2slf_dfmspace << "), EW(" << g2slf_dfmspace << ")), VECTOR(ECMIN("
          << end_orient << "), ECMAX(" << end_orient << "), " << sMaxSlf << "))]\n\n";

    std::string be_end_select = sTmpLayerPrefix;
    be_end_select += ".be_end.select";

    fSvrf << be_end_select << " = DFM PROPERTY " << be_end_combine << "\n [BE = RANGE_XXY(SORT_MERGE_XXY(VPROPERTY("
          << be_end_combine << ", \"BE_LAYER1_ENC\"), VPROPERTY(" << be_end_combine << ", \"BE_LAYER1_EXT\")), VPROPERTY("
          << be_end_combine << ", \"BE_RANGE\"))]\n\n";

    std::string lod_end_combine = sTmpLayerPrefix;
    lod_end_combine += ".lod_end.combine";

    fSvrf << lod_end_combine << " = DFM PROPERTY " << g2slfod_lat_sodlf << " "
          << g2slfod_dfmspace << " OVERLAP ABUT ALSO MULTI \n [LOD_DIFF_EXT = CONCAT(VECTOR(ECMIN("
          << g2slfod_dfmspace << "), ECMAX(" << g2slfod_dfmspace << "), EW(" << g2slfod_dfmspace << ")), VECTOR(ECMIN("
          << g2slfod_lat_sodlf << "), ECMAX(" << g2slfod_lat_sodlf << ")," << sMaxSodlf << "))]\n\n";

    std::string lod_end_select = sTmpLayerPrefix;
    lod_end_select += ".lod_end.select";

    fSvrf << lod_end_select << " = DFM PROPERTY " << end_orient << " " << lod_end_combine
          << " OVERLAP ABUT ALSO MULTI \n [LOD = RANGE_XXY(SORT_MERGE_XXY(VPROPERTY("
          << lod_end_combine << ", \"LOD_DIFF_EXT\")), VECTOR(ECMIN(" << end_orient << ") - "
          << std::setiosflags(std::ios::fixed) << fLatSodlf * 0.999999 << ", ECMAX(" << end_orient
          << ") + " << fLatSodlf * 0.999999 << "))] \n [LOD_RANGE = VECTOR(ECMIN(" << end_orient
          << "), ECMAX(" << end_orient << "))]\n\n";

    std::string end = sTmpLayerPrefix;
    end += ".end";

    fSvrf << end << " = DFM PROPERTY " << end_orient << " " << be_end_select << " " << lod_end_select
          << " OVERLAP ABUT ALSO MULTI \n [BE = VPROPERTY(" << be_end_select << ", \"BE\")]\n [LOD = VPROPERTY("
          << lod_end_select << ", \"LOD\")]\n [LOD_RANGE = VPROPERTY(" << lod_end_select << ", \"LOD_RANGE\")]";

    sValue.clear();

    sValue += "DFM PROPERTY ";
    sValue += sGateLayer;
    sValue += " ";
    sValue += side;
    sValue += " ";
    sValue += end;
    sValue += " OVERLAP ABUT ALSO \n [G2SP_P_1 = VPROPERTY(";
    sValue += side;
    sValue += ", PSE, 1)]\n [G2SP_P_2 = VPROPERTY(";
    sValue += side;
    sValue += ", PSE, 2)]\n [G2SP_E_1 = VECTOR()]\n [G2SP_E_1 = VECTOR()]\n [PER_1 = VPROPERTY(";
    sValue += side;
    sValue += ", BE, 1)]\n [PER_2 = VPROPERTY(";
    sValue += side;
    sValue += ", BE, 2)]\n [PAR_1 = VPROPERTY(";
    sValue += end;
    sValue += ", BE, 1)]\n [PAR_2 = VPROPERTY(";
    sValue += end;
    sValue += ", BE, 2)]\n [LPEPER_1 = VPROPERTY(";
    sValue += side;
    sValue += ", LOD, 1)]\n [LPEPER_2 = VPROPERTY(";
    sValue += side;
    sValue += ", LOD, 2)]\n [LPEPAR_1 = VPROPERTY(";
    sValue += end;
    sValue += ", LOD, 1)]\n [LPEPAR_1_RANGE = VPROPERTY(";
    sValue += end;
    sValue += ", LOD_RANGE, 1)]\n [LPEPAR_2 = VPROPERTY(";
    sValue += end;
    sValue += ", LOD, 2)]\n [LPEPAR_2_RANGE = VPROPERTY(";
    sValue += end;
    sValue += ", LOD_RANGE, 2)]\n\n";

    sValue += "\n";

    ++iLayer;

    return TCL_OK;
}

int
rcsTVFCompiler_T::strained_silicon_measurements_proc(ClientData d, Tcl_Interp *interp,
                                                     int argc, const char * argv[])
{
    std::string sMessage = "\nlvs : device::strained_silicon_measurements [options] \
options:\n\
 -gate value          Derived gate layer <>\n\
 -sd value            Derived source/drain layer <>\n\
 -active value        Drawn active area layer <>\n\
 -poly value          Drawn poly layer <>\n\
 -layer1 value        Derived mystery layer <>\n\
 -max_sp value        Gate to first poly spacing maximum search distance override [PSE] <-1.0>\n\
 -max_sp2 value       Gate to second poly spacing maximum search distance override [PSE] <-1.0>\n\
 -max_sd value        Gate to diffusion enclosure maximum search distance override [PSE] <-1.0>\n\
 -max_se value        Gate to layer1 enclosure maximum search distance override (sides) [BE] <-1.0>\n\
 -max_sf value        Gate to layer1 spacing maximum search distance override (sides) [BE] <-1.0>\n\
 -max_sode value      Gate to diffusion enclosure maximum search distance override (sides) [LOD] <-1.0>\n\
 -max_sodf value      Gate to diffusion spacing maximum search distance override (sides) [LOD] <-1.0>\n\
 -max_sle value       Gate to layer1 enclosure maximum search distance override (ends) [BE] <-1.0>\n\
 -max_slf value       Gate to layer1 spacing maximum search distance override (ends) [BE] <-1.0>\n\
 -max_sodlf value     Gate to diffusion spacing maximum search distance override (ends) [LOD] <-1.0>\n\
 -lat_sodlf value     Distance to search laterally from gate to nearest external diffusion (ends) [LOD] <-1.0>\n\
 -help                Print this message\n\
 -?                   Print this message\n\n";

    std::string sResult;
    if(TCL_ERROR == convertStrainedSiliconMeasurementsToDrcCmds(interp, argc, argv, sResult, m_fSvrf))
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}

static inline int
convertContactResistanceMeasurementsToDrcCmds(Tcl_Interp *interp, int argc, const char * argv[],
                                              std::string &sValue, std::ofstream &fSvrf)
{
    std::string sMessage = "tvf : device::contact_resistance_measurements [options] \
options:\
 -gate value          Derived gate layer <>\
 -sd value            Derived source/drain layer <>\
 -contact value       Contact layer <>\
 -max_ct value        Gate to contact maximum search distance override <-1.0>\
 -help                Print this message\
 -?                   Print this message";

    static hvUInt32 iLayer = 1;
    static std::string sBreak = "__";
    std::string sGateLayer;
    std::string sSdLayer;
    std::string sContactLayer;
    std::string sMaxCt;

    for(int iPara = 1; iPara < argc; ++iPara)
    {
        if(strcmp(argv[iPara], "-gate") == 0)
        {
            if(!sGateLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sGateLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-sd") == 0)
        {
            if(!sSdLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sSdLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-contact") == 0)
        {
            if(!sContactLayer.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sContactLayer = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-max_ct") == 0)
        {
            if(!sMaxCt.empty())
                return TCL_ERROR;

            if(++iPara < argc)
            {
                sMaxCt = argv[iPara];
            }
            else
            {
                return TCL_ERROR;
            }
        }
        else if(strcmp(argv[iPara], "-help") == 0 || strcmp(argv[iPara], "-?") == 0)
        {
            return TCL_ERROR;
        }
    }

    if(sMaxCt.empty())
    {
        const char *pValue = Tcl_GetVar(interp, "device::max_ct", TCL_NAMESPACE_ONLY);
        if(pValue != NULL)
            sMaxCt = pValue;
        else
            return TCL_ERROR;
    }

    if(sMaxCt.empty() || sGateLayer.empty() || sSdLayer.empty() || sContactLayer.empty())
        return TCL_ERROR;

    double fMaxCt;
    if(TCL_ERROR == Tcl_GetDouble(interp, sMaxCt.c_str(), &fMaxCt))
        return TCL_ERROR;

    std::string sTmpLayerPrefix;

    sTmpLayerPrefix += sGateLayer;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += sSdLayer;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += sContactLayer;
    sTmpLayerPrefix += sBreak;
    sTmpLayerPrefix += "cr__TVF_tmp_";
    sTmpLayerPrefix += itoa(iLayer);

    std::string contact = sTmpLayerPrefix;
    contact += ".contact";

    fSvrf << contact << " = AND " << sContactLayer << " " << sSdLayer << std::endl;

    std::string side = sTmpLayerPrefix;
    side += ".side";

    fSvrf << side << " = COINCIDENT EDGE " << sGateLayer << " " << sSdLayer << "\n";

    std::string gate2sd = sTmpLayerPrefix;
    gate2sd += ".gate2sd";

    fSvrf << gate2sd << " = DFM SPACE " << side << " " << sSdLayer
          << " <= " << sMaxCt << " BY ENC BY LAYER " << sSdLayer << " COUNT == 0 MEASURE ALL\n";

    std::string gate2ct_all = sTmpLayerPrefix;
    gate2ct_all += ".gate2ct_all";

    fSvrf << gate2ct_all << " = DFM SPACE " << side << " " << contact
          << " <= " << sMaxCt << " BY EXT BY LAYER " << contact << " COUNT == 0 \n";

    std::string sd_edge = sTmpLayerPrefix;
    sd_edge += ".sd_edge";

    fSvrf << sd_edge << " = LENGTH " << sSdLayer << " > 0\n";

    std::string gate2ct_same_sd = sTmpLayerPrefix;
    gate2ct_same_sd += ".gate2ct_same_sd";

    fSvrf << gate2ct_same_sd << " = DFM PROPERTY " << gate2ct_all << " " << sd_edge
          << " OVERLAP ABUT ALSO REGION MULTI [ -= COUNT(" << sd_edge << ") ] < 3\n";

    std::string cont_edge = sTmpLayerPrefix;
    cont_edge += ".cont_edge";

    fSvrf << cont_edge << " = LENGTH " << sContactLayer << " > 0\n";

    std::string cont_edge_p = sTmpLayerPrefix;
    cont_edge_p += ".cont_edge_p";

    fSvrf << cont_edge_p << " = DFM PROPERTY " << cont_edge << " [ ct_length = (ECMAX("
          << cont_edge << ")-ECMIN(" << cont_edge << "))/2 ]\n";

    std::string gate2ct = sTmpLayerPrefix;
    gate2ct += ".gate2ct";

    fSvrf << gate2ct << " = DFM PROPERTY " << gate2ct_same_sd << " " << cont_edge_p
          << " OVERLAP ABUT ALSO [ ct_length = PROPERTY(" << cont_edge_p << ", ct_length) ]\n";

    std::string sdnet = sTmpLayerPrefix;
    sdnet += ".sdnet";

    fSvrf << sdnet << " = DFM PROPERTY " << sSdLayer
          << " [ NETID = NETID( " << sSdLayer <<  ") ]\n";

    std::string sd_p = sTmpLayerPrefix;
    sd_p += ".sd_p";

    fSvrf << sd_p << " = DFM PROPERTY " << sSdLayer << " " << sGateLayer
          << " OVERLAP ABUT ALSO MULTI [ share_factor = COUNT(" << sGateLayer << ") ]\n";

    std::string sideprp = sTmpLayerPrefix;
    sideprp += ".sideprp";

    fSvrf << sideprp << " = DFM PROPERTY " << side << " " << gate2ct << " "
          << gate2sd << " " << sdnet << " " << sd_p << " OVERLAP ABUT ALSO MULTI [ SF = PROPERTY("
          << sd_p << ", share_factor) ]\n [ NET = NETPROPERTY(" << sdnet
          << ", NETID, 1) ]\n [ CT_RANGE = CONCAT(VECTOR(ECMIN(" << gate2ct << "), ECMAX("
          << gate2ct << "), PROPERTY(" << gate2ct << ", ct_length))) ]\n "
          << "[ G2SD = SORT_MERGE_XXY(VECTOR(ECMIN(" << gate2sd << "), ECMAX("
          << gate2sd << "), EW(" << gate2sd << "))) ]\n";

    sValue.clear();

    sValue += "DFM PROPERTY ";
    sValue += sGateLayer;
    sValue += " ";
    sValue += sideprp;
    sValue += " OVERLAP ABUT ALSO MULTI [ SF_1 = PROPERTY(";
    sValue += sideprp;
    sValue += " , SF, 1) ]\n [ NET_1 = NETPROPERTY(";
    sValue += sideprp;
    sValue += ", NET, 1) ]\n [ G2SD_1 = VPROPERTY(";
    sValue += sideprp;
    sValue += ", G2SD, 1) ]\n [ CT_RANGE1 = VPROPERTY(";
    sValue += sideprp;
    sValue += ", CT_RANGE, 1) ]\n  [ SF_2 = PROPERTY(";
    sValue += sideprp;
    sValue += ", SF, 2) ]\n [ NET_2 = NETPROPERTY(";
    sValue += sideprp;
    sValue += ", NET, 2) ]\n [ G2SD_2 = VPROPERTY(";
    sValue += sideprp;
    sValue += ", G2SD, 2) ]\n [ CT_RANGE2 = VPROPERTY(";
    sValue += sideprp;
    sValue += ", CT_RANGE, 2) ]\n";
    sValue += "\n";

    ++iLayer;

    return TCL_OK;
}

int
rcsTVFCompiler_T::contact_resistance_measurements_proc(ClientData d, Tcl_Interp *interp,
                                                       int argc, const char * argv[])
{
    std::string sMessage = "tvf : device::contact_resistance_measurements [options] \n\
options:\n\
 -gate value          Derived gate layer <>\n\
 -sd value            Derived source/drain layer <>\n\
 -contact value       Contact layer <>\n\
 -max_ct value        Gate to contact maximum search distance override <-1.0>\n\
 -help                Print this message\n\
 -?                   Print this message\n";

    std::string sResult;
    if(TCL_ERROR == convertContactResistanceMeasurementsToDrcCmds(interp, argc, argv, sResult, m_fSvrf))
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}


int
rcsTVFCompiler_T::void_proc(ClientData d, Tcl_Interp *interp,
                            int argc, const char * argv[])
{
    printf("Warning: unsupported TVF command \"%s\"\n",argv[0]);
    if(argc == 2)
    {
        return TCL_OK;
    }
    else
    {
        return TCL_ERROR;
    }
}


int
rcsTVFCompiler_T::setlayer_proc(ClientData d, Tcl_Interp *interp,
                                int argc, const char * argv[])
{
    if( argc == 3 )
    {
        if( strcmp( argv[1], "-getnames" ) == 0 )
        {
            std::string res = "";
            if( strcmp( argv[2], "-global" ) == 0 )
            {
                for(std::map<std::string, std::string, LTstr>::iterator it = layermap_g.begin();
                    it != layermap_g.end(); ++it)
                {
                    res = res + it->first + " ";
                }
                Tcl_SetResult(interp, const_cast<char*>(res.c_str()), NULL);
            }
            else
            if( strcmp( argv[2], "-local" ) == 0 )
            {
                for(std::map<std::string, std::string, LTstr>::iterator it = layermap_g.begin();
                    it != layermap_g.end(); ++it)
                {
                    res = res + it->first + " ";
                }
                for(std::map<std::string, std::string, LTstr>::iterator it = layermap_l.begin();
                    it != layermap_l.end(); ++it)
                {
                    res = res + it->first + " ";
                }

                Tcl_SetResult( interp, const_cast<char*>(res.c_str()), NULL );
            }
            else
            {
                printf("error argument for tvf::SETLAYER\n");
                return TCL_ERROR;
            }
        }
        else if( strcmp( argv[2],"-getexpr" ) == 0 )
        {
            std::map<std::string, std::string, LTstr>::iterator it = layermap_g.find(argv[1]);
            if( it != layermap_g.end() )
            {
                sprintf(interp->result,"{%s}",it->second.c_str());
            }
            else
            {
                it = layermap_l.find(argv[1]);
                if( it != layermap_l.end() )
                    sprintf(interp->result,"{%s}",it->second.c_str());
                else
                {
                    printf("%s is not defined\n",argv[1] );
                }
            }
        }
        else
        {
            printf("error argument for tvf::SETLAYER at:\n%s %s %s",argv[0],argv[1],argv[2]);
            return TCL_ERROR;
        }
    }
    else
    {
        Utassert( argc > 3 );
        if( strcmp( argv[2], "=" ) != 0 )
        {
            printf("\nTVF Error: the second argument of tvf::SETLAYER must be \"=\"!\n" );
            Tcl_SetResult( interp, const_cast<char*>("TVF Error: second argument of ::tvf::SETLAYER must be \"=\"!\n"), NULL );
            return TCL_ERROR;
        }
        std::string aa = "";
        if( m_isInRulecheck ) m_fSvrf << "\t";
        for( int i=1; i<argc; ++i)
        {
            if( i != 1 ) m_fSvrf << " " ;
            m_fSvrf << argv[i];
            if( i>2 ) aa += argv[i];
        }
        m_fSvrf << std::endl;
        std::string key = argv[1];         
        std::string buf = aa;
        if( m_isInRulecheck )
            layermap_l[key] = buf;
        else
            layermap_g[key] = buf;
    }
    return TCL_OK;
}


int
rcsTVFCompiler_T::get_tmp_layer_name_proc(ClientData d, Tcl_Interp *interp,
                                          int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be device::get_tmp_layer_name prefix\n";
    std::string sResult;
    if(argc != 2)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    sResult = argv[1];
    sResult += "__TVF_tmp_1";

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}

int
rcsTVFCompiler_T::dfm_vec_measurements_proc(ClientData d, Tcl_Interp *interp,
                                          int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be device::dfm_vec_measurements object\n";
    std::string sResult;
    if(argc != 2)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    sResult = argv[1];
    sResult += "__TVF_tmp_1";

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}

int
rcsTVFCompiler_T::scaled_dfm_vec_measurements_proc(ClientData d, Tcl_Interp *interp,
                                          int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be device::scaled_dfm_vec_measurements object scale_factor\n";
    std::string sResult;
    if(argc != 3)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    sResult = argv[1];
    sResult += "__TVF_tmp_1";

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}

int
rcsTVFCompiler_T::set_search_distance_proc(ClientData d, Tcl_Interp *interp,
                                           int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be \"device::set_search_distance gate sd_name global_sd override_sd\"\n";
    std::string sResult;
    if(argc != 5)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    sResult = argv[4];
    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}

int
rcsTVFCompiler_T::build_drc_layer_debug_proc(ClientData d, Tcl_Interp *interp,
                                             int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be \"device::build_drc_layer_debug measurement_layers checkname report\"\n";
    std::string sResult;
    if(argc != 4)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    std::string sMeasurementLayers = argv[1];
    std::string sCheckName         = argv[2];
    std::string sReportName        = argv[3];

    std::vector<std::string> vMeasurementLayers;
    tokenize(vMeasurementLayers, sMeasurementLayers);
    if(vMeasurementLayers.empty())
        return TCL_OK;

    for(std::vector<std::string>::const_iterator iter = vMeasurementLayers.begin();
        iter != vMeasurementLayers.end(); ++iter)
    {
        sResult += "DFM RDB ";
        sResult += *iter;
        sResult += " ";
        sResult += sReportName;
        sResult += ".rep CHECKNAME ";
        sResult += sCheckName;
        sResult += "_LAYER.";
        sResult += *iter;
        sResult += "\n";
    }

    m_fSvrf << sResult;

    return TCL_OK;
}

int
rcsTVFCompiler_T::build_dfm_layer_debug_proc(ClientData d, Tcl_Interp *interp,
                                             int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be \"device::build_drc_layer_debug measurement_layers checkname report\"\n";
    std::string sResult;
    if(argc != 4)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    std::string sMeasurementLayer = argv[1];
    std::string sCheckName        = argv[2];
    std::string sReportName       = argv[3];

    sResult += "DFM RDB ";
    sResult += sMeasurementLayer;
    sResult += ".combine ";
    sResult += sReportName;
    sResult += ".rep CHECKNAME ";
    sResult += sCheckName;
    sResult += "_LAYER.COMBINE";
    sResult += "\n";

    sResult += "DFM RDB ";
    sResult += sMeasurementLayer;
    sResult += ".select ";
    sResult += sReportName;
    sResult += ".rep CHECKNAME ";
    sResult += sCheckName;
    sResult += "_LAYER.SELECT";
    sResult += "\n";

    m_fSvrf << sResult;

    return TCL_OK;
}

int
rcsTVFCompiler_T::build_nlayer_concat_vectors_string_proc(ClientData d, Tcl_Interp *interp,
                                                          int argc, const char * argv[])
{
    std::string sMessage = "wrong # args: should be \"device::build_nlayer_concat_vectors_string measurement_layers maxsd base_ec ?ew_correction?\"\n";
    std::string sResult;
    if(argc != 4 && argc != 5)
    {
        Tcl_Obj *pResult = Tcl_NewStringObj(sMessage.c_str(), sMessage.size());
        Tcl_SetObjResult(interp, pResult);

        return TCL_ERROR;
    }

    std::string sMeasurementLayers = argv[1];
    std::string sMaxSd             = argv[2];
    std::string sBaseEcLayer       = argv[3];
    std::string sEwCorrection      = "0";
    if(argc == 5)
        sEwCorrection = argv[4];

    std::vector<std::string> vMeasurementLayers;
    tokenize(vMeasurementLayers, sMeasurementLayers);
    if(vMeasurementLayers.empty())
        return TCL_OK;

    sResult += " CONCAT(\n";
    for(std::vector<std::string>::const_iterator iter = vMeasurementLayers.begin();
        iter != vMeasurementLayers.end(); ++iter)
    {
        sResult += "VECTOR(ECMIN(";
        sResult += *iter;
        sResult += "), ECMAX(";
        sResult += *iter;
        sResult += "),EW(";
        sResult += *iter;
        sResult += ")+";
        sResult += sEwCorrection;
        sResult += ")\n";
    }

    sResult += "VECTOR(ECMIN(";
    sResult += sBaseEcLayer;
    sResult += "), ECMAX(";
    sResult += sBaseEcLayer;
    sResult += "),";
    sResult += sMaxSd;
    sResult += "))\n";

    Tcl_Obj *pResult = Tcl_NewStringObj(sResult.c_str(), sResult.size());
    Tcl_SetObjResult(interp, pResult);

    return TCL_OK;
}


int
rcsTVFCompiler_T::InitProc( Tcl_Interp *interp )
{
    int iRet = Tcl_Init( interp );
    if( iRet == TCL_ERROR )
    {
        fprintf(stderr, "Unable to Initialize Tcl: %s\n", interp->result);
        return( iRet );
    }

    const char* tvfCmd =
        "namespace eval tvf { \
        namespace export // @ COMMENT comment echo_to_svrf_file output_to_svrf get_tvf_arg OUTLAYER outlayer echo_svrf; \
        namespace export RULECHECK rulecheck set_disclaimer set_rule_check_indentation set_traceback_line SETLAYER setlayer VERBATIM verbatim ; \
        namespace export ATTACH FLAG LVS SNAP CAPACITANCE FLATTEN MASK SOURCE CONNECT FRACTURE PEX TEXT INCLUDE; \
        namespace export COPY GROUP POLYGON TITLE DEVICE HCELL PORT TRACE DISCONNECT PRECISION TVF DRC DFM LABEL PUSH UNIT ERC; \
        namespace export LAYER RESISTANCE VARIABLE EXCLUDE LAYOUT RESOLUTION VIRTUAL EXPAND LITHO SCONNECT; \
        namespace export attach flag lvs snap capacitance flatten mask connect fracture pex text include; \
        namespace export copy group polygon title device hcell port disconnect precision tvf drc dfm label push erc; \
        namespace export layer resistance exclude layout resolution virtual expand litho sconnect; \
        namespace export exists_global_variable EXISTS_GLOBAL_VARIABLE get_global_variable GET_GLOBAL_VARIABLE;\
        namespace export set_global_variable SET_GLOBAL_VARIABLE unset_global_variable UNSET_GLOBAL_VARIABLE;\
        }";

    iRet = Tcl_RecordAndEval( interp, const_cast<char*>(tvfCmd), 0);
    if( iRet == TCL_ERROR )
    {
        printf("define tvf namespace failed!\n");
        return TCL_ERROR;
    }

    Tcl_CreateCommand(interp,"tvf:://",(&rcsTVFCompiler_T::blockcomment_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::@",(&rcsTVFCompiler_T::linecomment_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::comment",(&rcsTVFCompiler_T::linecomment_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::COMMENT",(&rcsTVFCompiler_T::linecomment_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::echo_to_svrf_file",(&rcsTVFCompiler_T::echo_to_svrf_file_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::output_to_svrf",(&rcsTVFCompiler_T::output_to_svrf_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::get_tvf_arg",(&rcsTVFCompiler_T::gettvfarg_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::echo_svrf",(&rcsTVFCompiler_T::void_proc),(ClientData)0,0);

    Tcl_CreateCommand(interp,"tvf::outlayer",(&rcsTVFCompiler_T::print_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::OUTLAYER",(&rcsTVFCompiler_T::print_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::rulecheck",(&rcsTVFCompiler_T::rulecheck_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::RULECHECK",(&rcsTVFCompiler_T::rulecheck_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::set_rule_check_indentation",(&rcsTVFCompiler_T::void_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::set_disclaimer",(&rcsTVFCompiler_T::void_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::set_traceback_line",(&rcsTVFCompiler_T::void_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::setlayer",(&rcsTVFCompiler_T::setlayer_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::SETLAYER",(&rcsTVFCompiler_T::setlayer_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::verbatim",(&rcsTVFCompiler_T::verbatim_proc),(ClientData)0,0);
    Tcl_CreateCommand(interp,"tvf::VERBATIM",(&rcsTVFCompiler_T::verbatim_proc),(ClientData)0,0);

    Tcl_CreateCommand(interp, "tvf::exists_global_variable", (&rcsTVFCompiler_T::exists_global_var_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "tvf::get_global_variable", (&rcsTVFCompiler_T::get_global_var_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "tvf::set_global_variable", (&rcsTVFCompiler_T::set_global_var_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "tvf::unset_global_variable", (&rcsTVFCompiler_T::unset_global_var_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp,"tvf::attach",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"ATTACH",0);
    Tcl_CreateCommand(interp,"tvf::flag",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"FLAG",0);
    Tcl_CreateCommand(interp,"tvf::lvs",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LVS",0);
    Tcl_CreateCommand(interp,"tvf::snap",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"SNAP",0);
    Tcl_CreateCommand(interp,"tvf::capacitance",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"CAPACITANCE",0);
    Tcl_CreateCommand(interp,"tvf::flatten",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"FLATTEN",0);
    Tcl_CreateCommand(interp,"tvf::mask",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"MASK",0);
    Tcl_CreateCommand(interp,"tvf::connect",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"CONNECT",0);
    Tcl_CreateCommand(interp,"tvf::fracture",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"FRACTURE",0);
    Tcl_CreateCommand(interp,"tvf::pex",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PEX",0);
    Tcl_CreateCommand(interp,"tvf::text",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"TEXT",0);
    Tcl_CreateCommand(interp,"tvf::copy",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"COPY",0);
    Tcl_CreateCommand(interp,"tvf::group",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"GROUP",0);
    Tcl_CreateCommand(interp,"tvf::polygon",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"POLYGON",0);
    Tcl_CreateCommand(interp,"tvf::title",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"TITLE",0);
    Tcl_CreateCommand(interp,"tvf::device",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DEVICE",0);
    Tcl_CreateCommand(interp,"tvf::hcell",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"HCELL",0);
    Tcl_CreateCommand(interp,"tvf::port",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PORT",0);
    Tcl_CreateCommand(interp,"tvf::disconnect",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DISCONNECT",0);
    Tcl_CreateCommand(interp,"tvf::precision",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PRECISION",0);
    Tcl_CreateCommand(interp,"tvf::tvf",(&rcsTVFCompiler_T::tvf_proc),(char*)"TVF",0);
    Tcl_CreateCommand(interp,"tvf::drc",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DRC",0);
    Tcl_CreateCommand(interp,"tvf::label",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LABEL",0);
    Tcl_CreateCommand(interp,"tvf::push",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PUSH",0);
    Tcl_CreateCommand(interp,"tvf::erc",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"ERC",0);
    Tcl_CreateCommand(interp,"tvf::layer",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LAYER",0);
    Tcl_CreateCommand(interp,"tvf::resistance",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"RESISTANCE",0);
    Tcl_CreateCommand(interp,"tvf::exclude",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"EXCLUDE",0);
    Tcl_CreateCommand(interp,"tvf::layout",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LAYOUT",0);
    Tcl_CreateCommand(interp,"tvf::resolution",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"RESOLUTION",0);
    Tcl_CreateCommand(interp,"tvf::virtual",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"VIRTUAL",0);
    Tcl_CreateCommand(interp,"tvf::expand",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"EXPAND",0);
    Tcl_CreateCommand(interp,"tvf::litho",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LITHO",0);
    Tcl_CreateCommand(interp,"tvf::sconnect",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"SCONNECT",0);

    Tcl_CreateCommand(interp,"tvf::ATTACH",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"ATTACH",0);
    Tcl_CreateCommand(interp,"tvf::FLAG",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"FLAG",0);
    Tcl_CreateCommand(interp,"tvf::LVS",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LVS",0);
    Tcl_CreateCommand(interp,"tvf::SNAP",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"SNAP",0);
    Tcl_CreateCommand(interp,"tvf::CAPACITANCE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"CAPACITANCE",0);
    Tcl_CreateCommand(interp,"tvf::FLATTEN",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"FLATTEN",0);
    Tcl_CreateCommand(interp,"tvf::MASK",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"MASK",0);
    Tcl_CreateCommand(interp,"tvf::SOURCE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"SOURCE",0);
    Tcl_CreateCommand(interp,"tvf::CONNECT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"CONNECT",0);
    Tcl_CreateCommand(interp,"tvf::FRACTURE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"FRACTURE",0);
    Tcl_CreateCommand(interp,"tvf::PEX",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PEX",0);
    Tcl_CreateCommand(interp,"tvf::TEXT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"TEXT",0);
    Tcl_CreateCommand(interp,"tvf::COPY",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"COPY",0);
    Tcl_CreateCommand(interp,"tvf::GROUP",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"GROUP",0);
    Tcl_CreateCommand(interp,"tvf::POLYGON",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"POLYGON",0);
    Tcl_CreateCommand(interp,"tvf::TITLE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"TITLE",0);
    Tcl_CreateCommand(interp,"tvf::DEVICE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DEVICE",0);
    Tcl_CreateCommand(interp,"tvf::HCELL",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"HCELL",0);
    Tcl_CreateCommand(interp,"tvf::PORT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PORT",0);
    Tcl_CreateCommand(interp,"tvf::TRACE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"TRACE",0);
    Tcl_CreateCommand(interp,"tvf::DISCONNECT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DISCONNECT",0);
    Tcl_CreateCommand(interp,"tvf::PRECISION",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PRECISION",0);
    Tcl_CreateCommand(interp,"tvf::TVF",(&rcsTVFCompiler_T::tvf_proc),(char*)"TVF",0);
    Tcl_CreateCommand(interp,"tvf::DRC",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DRC",0);
    Tcl_CreateCommand(interp,"tvf::LABEL",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LABEL",0);
    Tcl_CreateCommand(interp,"tvf::PUSH",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"PUSH",0);
    Tcl_CreateCommand(interp,"tvf::UNIT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"UNIT",0);
    Tcl_CreateCommand(interp,"tvf::ERC",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"ERC",0);
    Tcl_CreateCommand(interp,"tvf::LAYER",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LAYER",0);
    Tcl_CreateCommand(interp,"tvf::RESISTANCE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"RESISTANCE",0);
    Tcl_CreateCommand(interp,"tvf::VARIABLE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"VARIABLE",0);
    Tcl_CreateCommand(interp,"tvf::EXCLUDE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"EXCLUDE",0);
    Tcl_CreateCommand(interp,"tvf::LAYOUT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LAYOUT",0);
    Tcl_CreateCommand(interp,"tvf::RESOLUTION",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"RESOLUTION",0);
    Tcl_CreateCommand(interp,"tvf::VIRTUAL",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"VIRTUAL",0);
    Tcl_CreateCommand(interp,"tvf::EXPAND",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"EXPAND",0);
    Tcl_CreateCommand(interp,"tvf::LITHO",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"LITHO",0);
    Tcl_CreateCommand(interp,"tvf::SCONNECT",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"SCONNECT",0);

    Tcl_CreateCommand(interp,"tvf::DFM",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"DFM",0);
    Tcl_CreateCommand(interp,"tvf::dfm",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"dfm",0);

    Tcl_CreateCommand(interp,"tvf::INCLUDE",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"INCLUDE",0);
    Tcl_CreateCommand(interp,"tvf::include",(&rcsTVFCompiler_T::svrfcmd_proc),(char*)"include",0);

    const char* deviceCmd =
        "namespace eval device { \
        namespace export enclosure_measurements ENCLOSURE_MEASUREMENTS strained_silicon_measurements STRAINED_SILICON_MEASUREMENTS contact_resistance_measurements CONTACT_RESISTANCE_MEASUREMENTS; \
        namespace export get_tmp_layer_name GET_TMP_LAYER_NAME set_search_distance SET_SEARCH_DISTANCE build_drc_layer_debug BUILD_DRC_LAYER_DEBUG build_dfm_layer_debug BUILD_DFM_LAYER_DEBUG build_nlayer_concat_vectors_string BUILD_NLAYER_CONCAT_VECTORS_STRING;\
        }";
    iRet = Tcl_RecordAndEval(interp, const_cast<char*>(deviceCmd), 0);
    if(iRet == TCL_ERROR)
    {
        printf("define device namespace failed!\n");
        return TCL_ERROR;
    }

    Tcl_SetVar(interp, "device::input_debug", "0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::side_orient_layer_debug", "0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::end_orient_layer_debug", "0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::g2sp_layer_debug", "0", TCL_NAMESPACE_ONLY);
	Tcl_SetVar(interp, "device::g2sd_layer_debug", "0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::pse_layer_debug", "0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::side_layer_debug", "0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::all_pse_debug", "0", TCL_NAMESPACE_ONLY);

    Tcl_SetVar(interp, "device::max_sp", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_sp2", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_sd", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_ct", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_sode", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_sodf", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_sle", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_slf", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_sodlf", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::lat_sodlf", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_par", "-1.0", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::max_per", "-1.0", TCL_NAMESPACE_ONLY);

    Tcl_SetVar(interp, "device::avg_sp3", "0.5", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::avg_enc", "1.5", TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "device::dfmspace", "1", TCL_NAMESPACE_ONLY);

    Tcl_CreateCommand(interp,"device::enclosure_measurements", (&rcsTVFCompiler_T::enclosure_measurements_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp,"device::ENCLOSURE_MEASUREMENTS", (&rcsTVFCompiler_T::enclosure_measurements_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp,"device::strained_silicon_measurements", (&rcsTVFCompiler_T::strained_silicon_measurements_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp,"device::STRAINED_SILICON_MEASUREMENTS", (&rcsTVFCompiler_T::strained_silicon_measurements_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp,"device::contact_resistance_measurements", (&rcsTVFCompiler_T::contact_resistance_measurements_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp,"device::CONTACT_RESISTANCE_MEASUREMENTS", (&rcsTVFCompiler_T::contact_resistance_measurements_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::get_tmp_layer_name", (&rcsTVFCompiler_T::get_tmp_layer_name_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::GET_TMP_LAYER_NAME", (&rcsTVFCompiler_T::get_tmp_layer_name_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::set_search_distance", (&rcsTVFCompiler_T::set_search_distance_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::SET_SEARCH_DISTANCE", (&rcsTVFCompiler_T::set_search_distance_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::build_drc_layer_debug", (&rcsTVFCompiler_T::build_drc_layer_debug_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::BUILD_DRC_LAYER_DEBUG", (&rcsTVFCompiler_T::build_drc_layer_debug_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::build_dfm_layer_debug", (&rcsTVFCompiler_T::build_dfm_layer_debug_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::BUILD_DFM_LAYAR_DEBUG", (&rcsTVFCompiler_T::build_dfm_layer_debug_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::build_nlayer_concat_vectors_string",  (&rcsTVFCompiler_T::build_nlayer_concat_vectors_string_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::BUILD_NLAYER_CONCAT_VECTORS_STRING",  (&rcsTVFCompiler_T::build_nlayer_concat_vectors_string_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::convert_to_scaled_enc",  (&rcsTVFCompiler_T::scaled_dfm_vec_measurements_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::CONVERT_TO_SCALED_ENC",  (&rcsTVFCompiler_T::scaled_dfm_vec_measurements_proc), (ClientData)0, 0);

    Tcl_CreateCommand(interp, "device::convert_to_enc",  (&rcsTVFCompiler_T::dfm_vec_measurements_proc), (ClientData)0, 0);
    Tcl_CreateCommand(interp, "device::CONVERT_TO_ENC",  (&rcsTVFCompiler_T::dfm_vec_measurements_proc), (ClientData)0, 0);

    return TCL_OK;
}


int
rcsTVFCompiler_T::tvf_compiler()
{
    if(access(m_pTvfFilename, R_OK) != 0)
    {
        printf("no file %s exist or has no permission to read\n!", m_pTvfFilename );
        return TCL_ERROR;
    }

#if 0 
    std::ifstream reader(m_pTvfFilename);
    if(!reader)
        return TCL_ERROR;

    std::ofstream outer(".mergetvf.cmd");
    std::string sLineBuf;
    while(std::getline(reader, sLineBuf))
    {
        std::string sValue = sLineBuf;
        trim(sLineBuf);
        
        if(sLineBuf.size() >= 23 && isTclPackageLine(sLineBuf) && std::string::npos != sLineBuf.find("cmdline"))
        {
        }
        else if(isTclPackageLine(sLineBuf))
        {
            sLineBuf.insert(0, "#");
        }
        outer << sLineBuf << std::endl;
    }
    outer.close();
#endif

    m_fSvrf.open(m_pSvrfFileName);
    if(m_fSvrf.fail())
    {
        printf("can't open %s to write\n", m_pSvrfFileName);
        return TCL_ERROR;
    }

    int iRet = Tcl_EvalFile(m_pInterp, m_pTvfFilename);
    if(iRet == TCL_ERROR)
    {
        printf("\nERROR occurs at line %d: %s\n", m_pInterp->errorLine,
               Tcl_GetString(Tcl_GetObjResult(m_pInterp)));
        return TCL_ERROR;
    }


    return TCL_OK;
}



rcsTVFCompiler_T::~rcsTVFCompiler_T()
{
    Tcl_DeleteInterp( m_pInterp );
    m_fSvrf.close();
}



rcsTVFCompiler_T::rcsTVFCompiler_T(char* pRuleFile, char* pSvrfFile, char* ptvfarg)
{
    m_pTvfFilename  = pRuleFile;
    m_pSvrfFileName = pSvrfFile;
    m_pTvfArg       = ptvfarg;

    if( NULL == m_pSvrfFileName )
    {
        m_pSvrfFileName = const_cast<char*>(".tvf2svrf.tmp");
    }

    m_pInterp = g_warpper_CreateInterp();
    int iret = InitProc( m_pInterp );
    if(iret == TCL_ERROR)
    {
        printf("Error to init TVF-Preprocessor\n");
        return;
    }

    return ;
}

char*
rcsTVFCompiler_T::getsvrffilename()
{
    return m_pSvrfFileName;
}
