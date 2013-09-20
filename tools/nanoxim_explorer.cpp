#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <cassert>
#include <cstdlib>
#include <sys/time.h>
#include <cstdlib>
#include <cstring>
using namespace std;

//---------------------------------------------------------------------------

#define DEFAULT_KEY          "default"
#define AGGREGATION_KEY      "aggregation"
#define EXPLORER_KEY         "explorer"
#define SIMULATOR_LABEL      "simulator"
#define REPETITIONS_LABEL    "repetitions"
#define PLOT_TYPE_LABEL      "plot_type"
#define TMP_DIR_LABEL        "tmp"

#define DEF_SIMULATOR        "./nanoxim"
#define DEF_REPETITIONS      5
#define DEF_TMP_DIR          "./"
#define DEF_PLOT_TYPE        0

#define PLOT_SET1	1
#define PLOT_SET2	2
#define PLOT_SET3	3


#define TMP_FILE_NAME        ".nanoxim_explorer.tmp"
#define RES_FILE_NAME        "results.txt"

#define DEFECTIVE_NODES_LABEL   "defective nodes:"
#define NODE_COVERAGE_LABEL 	"node coverage:"
#define LINK_COVERAGE_LABEL	"link coverage:"
#define NUMBER_OF_SEG_LABEL	"number of segments:"
#define AVERAGE_SEG_LENGTH_LABEL	"average segment length:"
#define LATENCY_LABEL 	"latency:"

#define MATLAB_VAR_NAME      "data"
#define MATRIX_COLUMN_WIDTH  15

//---------------------------------------------------------------------------

typedef unsigned int uint;

// parameter values
typedef vector<string> TParameterSpace;

// parameter name, parameter space
typedef map<string, TParameterSpace> TParametersSpace;

// parameter name, parameter value
typedef vector<pair<string, string> > TConfiguration;

typedef vector<TConfiguration> TConfigurationSpace;

struct TExplorerParams
{
  string simulator;
  string tmp_dir;
  int    repetitions;
  int plot_type;
};

struct TSimulationResults
{
	int covered_nodes;
	int covered_links;
	double node_coverage;
	double link_coverage;
	int nsegments;
	double avg_seg_length;
	int latency;
};

//---------------------------------------------------------------------------

double GetCurrentTime()
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_sec + (tv.tv_usec * 1.0e-6);
}

//---------------------------------------------------------------------------

void TimeToFinish(double elapsed_sec,
		  int completed, int total,
		  int& hours, int& minutes, int &seconds)
{
  double total_time_sec = (elapsed_sec * total)/completed;
  double remain_time_sec = total_time_sec - elapsed_sec;

  seconds = (int)remain_time_sec % 60;
  minutes = ((int)remain_time_sec / 60) % 60;
  hours   = (int)remain_time_sec / 3600;
}

//---------------------------------------------------------------------------

bool IsComment(const string& s)
{
  return (s == "" || s.at(0) == '%');
}

//---------------------------------------------------------------------------

string TrimLeftAndRight(const string& s)
{
  int len = s.length();
  
  int i, j;
  for (i=0; i<len && s.at(i) == ' '; i++) ;
  for (j=len-1; j>=0 && s.at(j) == ' '; j--) ;
  
  return s.substr(i,j-i+1);
}

//---------------------------------------------------------------------------

bool ExtractParameter(const string& s, string& parameter)
{
  long int i = s.find("[");

  if (i != string::npos)
    {
      long int j = s.rfind("]");
      
      if (j != string::npos)
	{
	  parameter = s.substr(i+1, j-i-1);
	  return true;
	}
    }

  return false;
}

//---------------------------------------------------------------------------

bool GetNextParameter(ifstream& fin, string& parameter)
{
  bool found = false;

  while (!fin.eof() && !found)
    {
      string s;
      getline(fin, s);

      if (!IsComment(s))
	found = ExtractParameter(s, parameter);
    }

  return found;
}

//---------------------------------------------------------------------------w

string MakeStopParameterTag(const string& parameter)
{
  string sparameter = "[/" + parameter + "]";

  return sparameter;
}

//---------------------------------------------------------------------------

bool ManagePlainParameterSet(ifstream& fin,
			     const string& parameter, 
			     TParametersSpace& params_space,
			     string& error_msg)
{
  string str_stop = MakeStopParameterTag(parameter);
  bool   stop = false;

  while (!fin.eof() && !stop)
    {
      string s;
      getline(fin, s);
      
      if (!IsComment(s))
	{
	  if (s.find(str_stop) != string::npos)
	    stop = true;
	  else
	    params_space[parameter].push_back(TrimLeftAndRight(s));
	}
    }
  
  return true;
}

//---------------------------------------------------------------------------

bool ExpandInterval(const string& sint,
		    TParameterSpace& ps,
		    string& error_msg)
{
  istringstream iss(sint);

  double min, max, step;

  iss >> min;
  iss >> max;
  iss >> step;

  string param_suffix;
  getline(iss, param_suffix);

  for (double v=min; v<=max; v+=step)
    {
      ostringstream oss;
      oss << v;
      ps.push_back(oss.str() + param_suffix);
    }

  return true;
}

//---------------------------------------------------------------------------

bool ManageCompressedParameterSet(ifstream& fin,
				  const string& parameter, 
				  TParametersSpace& params_space,
				  string& error_msg)
{
  string str_stop = MakeStopParameterTag(parameter);
  bool   stop = false;

  while (!fin.eof() && !stop)
    {
      string s;
      getline(fin, s);
      
      if (!IsComment(s))
	{
	  if (s.find(str_stop) != string::npos)
	    stop = true;
	  else	  
	    {
	      if (!ExpandInterval(s, params_space[parameter], error_msg))
		return false;
	    }
	}
    }
  
  return true;
}

//---------------------------------------------------------------------------

bool ManageParameter(ifstream& fin,
		     const string& parameter, 
		     TParametersSpace& params_space,
		     string& error_msg)
{
  bool err;

  if (parameter == "bootstrap" || parameter == "defective_nodes" )
    err = ManageCompressedParameterSet(fin, parameter, params_space, error_msg);
  else
    err = ManagePlainParameterSet(fin, parameter, params_space, error_msg);

  return err;
}

//---------------------------------------------------------------------------

bool ParseConfigurationFile(const string& fname,
			    TParametersSpace& params_space,
			    string& error_msg)
{
  ifstream fin(fname.c_str(), ios::in);

  if (!fin)
    {
      error_msg = "Cannot open " + fname;
      return false;
    }

  while (!fin.eof())
    {
      string parameter;

      if ( GetNextParameter(fin, parameter) )
	{
	  if (!ManageParameter(fin, parameter, params_space, error_msg))
	    return false;
	}
    }

  return true;
}

//---------------------------------------------------------------------------

bool LastCombination(const vector<pair<int,int> >& indexes)
{
  for (uint i=0; i<indexes.size(); i++)
    if (indexes[i].first < indexes[i].second-1)
      return false;

  return true;
}

//---------------------------------------------------------------------------

bool IncrementCombinatorialIndexes(vector<pair<int,int> >& indexes)
{
  for (uint i=0; i<indexes.size(); i++)
    {
      if (indexes[i].first < indexes[i].second - 1)
	{
	  indexes[i].first++;
	  return true;
	}
      indexes[i].first = 0;	
    }

  return false;
}

//---------------------------------------------------------------------------

TConfigurationSpace Explore(const TParametersSpace& params_space)

{
  TConfigurationSpace conf_space;
  
  vector<pair<int,int> > indexes; // <index, max_index>
  
  for (TParametersSpace::const_iterator psi=params_space.begin();
       psi!=params_space.end(); psi++)
      indexes.push_back(pair<int,int>(0, psi->second.size()));
  
  do 
    {
      int i = 0;
      TConfiguration conf;
      for (TParametersSpace::const_iterator psi=params_space.begin();
	   psi!=params_space.end(); psi++)
	{
	  conf.push_back( pair<string,string>(psi->first, 
					      psi->second[indexes[i].first]));	  
	  i++;
	}
      conf_space.push_back(conf);
    } 
  while (IncrementCombinatorialIndexes(indexes));

  return conf_space;
}

//---------------------------------------------------------------------------

bool RemoveParameter(TParametersSpace& params_space, 
		     const string& param_name,
		     TParameterSpace& param_space,
		     string& error_msg)
{
  TParametersSpace::iterator i = params_space.find(param_name);

  if (i == params_space.end())
    {
      error_msg = "Cannot extract parameter '" + param_name + "'";
      return false;
    }

  param_space = params_space[param_name];
  params_space.erase(i);
  
  return true;
}

//---------------------------------------------------------------------------

bool RemoveAggregateParameters(TParametersSpace& params_space, 
			       TParameterSpace&  aggregated_params,
			       TParametersSpace& aggragated_params_space,
			       string& error_msg)
{
  for (uint i=0; i<aggregated_params.size(); i++)
    {
      string param_name = aggregated_params[i];
      TParameterSpace param_space;
      if (!RemoveParameter(params_space, param_name, param_space, error_msg))
	return false;

      aggragated_params_space[param_name] = param_space;
    }

  return true;
}

//---------------------------------------------------------------------------

string ParamValue2Cmd(const pair<string,string>& pv)
{
  string cmd;

  if (pv.first == "topology")
    {
      istringstream iss(pv.second);

      int  width, height;
      char times;
      iss >> width >> times >> height;

      ostringstream oss;
      oss << "-dimx " << width << " -dimy " << height;

      cmd = oss.str();
    }
  else
    cmd = "-" + pv.first + " " + pv.second;

  return cmd;
}

//---------------------------------------------------------------------------

string Configuration2CmdLine(const TConfiguration& conf)
{
  string cl;

  for (uint i=0; i<conf.size(); i++)
    cl = cl + ParamValue2Cmd(conf[i]) + " ";
  
  return cl;
}

//---------------------------------------------------------------------------

string Configuration2FunctionName(const TConfiguration& conf)
{
  string fn;

  for (uint i=0; i<conf.size(); i++)
    fn = fn + conf[i].first + "_" + conf[i].second + "__";
  
  // Replace " ", "-", ".", "/" with "_"
  int len = fn.length();
  for (int i=0; i<len; i++)
    if (fn.at(i) == ' ' || fn.at(i) == '.' || fn.at(i) == '-' || fn.at(i) == '/')
      fn[i] = '_';
  
  return fn;
}

//---------------------------------------------------------------------------

bool ExtractExplorerParams(const TParameterSpace& explorer_params,
			   TExplorerParams& eparams,
			   string& error_msg)
{
  eparams.simulator   = DEF_SIMULATOR;
  eparams.tmp_dir     = DEF_TMP_DIR;
  eparams.repetitions = DEF_REPETITIONS;
  eparams.plot_type = DEF_PLOT_TYPE;

  for (uint i=0; i<explorer_params.size(); i++)
    {
      istringstream iss(explorer_params[i]);

      string label;
      iss >> label;
      
      if (label == SIMULATOR_LABEL)
	iss >> eparams.simulator;
      else if (label == REPETITIONS_LABEL)
	iss >> eparams.repetitions;
      else if (label == TMP_DIR_LABEL)
	iss >> eparams.tmp_dir;
      else if (label == PLOT_TYPE_LABEL)
	iss >> eparams.plot_type;
      else
	{
	  error_msg = "Invalid explorer option '" + label + "'";
	  return false;
	}
    }

  return true;
}

//---------------------------------------------------------------------------

bool PrintHeader(const string& fname,
		 const TExplorerParams& eparams,
		 const string& def_cmd_line, const string& conf_cmd_line, 
		 ofstream& fout, 
		 string& error_msg)
{
  fout.open(fname.c_str(), ios::out);
  if (!fout)
    {
      error_msg = "Cannot create " + fname;
      return false;
    }

  fout << "% fname: " << fname << endl
       << "% " << eparams.simulator << " "
       << conf_cmd_line << " " << def_cmd_line
       << endl << endl;

  return true;
}

//---------------------------------------------------------------------------

bool PrintMatlabFunction(const string& mfname,
			 ofstream& fout, 
			 string& error_msg)
{
  fout << "function [node_coverage, link_coverage, nsegments, avg_seg_length, latency] = " << mfname << "(symbol)" << endl
       << endl;

  return true;
}

//---------------------------------------------------------------------------

bool ReadResults(const string& fname, 
		 TSimulationResults& sres, 
		 string& error_msg)
{
  ifstream fin(fname.c_str(), ios::in);
  if (!fin)
    {
      error_msg = "Cannot read " + fname;
      return false;
    }

  int nread = 0;
  while (!fin.eof())
    {
      string line;
      getline(fin, line);

      long int  pos;
      
      pos = line.find(NODE_COVERAGE_LABEL);
      if (pos != string::npos) 
	{
	  nread++;
	  istringstream iss(line.substr(pos + string(NODE_COVERAGE_LABEL).size()));
	  iss >> sres.node_coverage;
	  continue;
	}

      pos = line.find(LINK_COVERAGE_LABEL);
      if (pos != string::npos) 
	{
	  nread++;
	  istringstream iss(line.substr(pos + string(LINK_COVERAGE_LABEL).size()));
	  iss >> sres.link_coverage;
	  continue;
	}

      pos = line.find(NUMBER_OF_SEG_LABEL);
      if (pos != string::npos) 
	{
	  nread++;
	  istringstream iss(line.substr(pos + string(NUMBER_OF_SEG_LABEL).size()));
	  iss >> sres.nsegments;
	  continue;
	}

      pos = line.find(AVERAGE_SEG_LENGTH_LABEL);
      if (pos != string::npos) 
	{
	  nread++;
	  istringstream iss(line.substr(pos + string(AVERAGE_SEG_LENGTH_LABEL).size()));
	  iss >> sres.avg_seg_length;
	  continue;
	}
      pos = line.find(LATENCY_LABEL);
      if (pos != string::npos) 
	{
	  nread++;
	  istringstream iss(line.substr(pos + string(LATENCY_LABEL).size()));
	  iss >> sres.latency;
	  continue;
	}
    }

  if (nread != 5)
    {
	cout << "\n nread = " << nread;
      error_msg = "Output file " + fname + " corrupted";
      return false;
    }

  return true;
}

//---------------------------------------------------------------------------

bool RunSimulation(const string& cmd_base,
		   const string& tmp_dir,
		   TSimulationResults& sres, 
		   string& error_msg)
{
  string tmp_fname = tmp_dir + TMP_FILE_NAME;
  //  string cmd = cmd_base + " >& " + tmp_fname; // this works only with csh and bash
  string cmd = cmd_base + " >" + tmp_fname + " 2>&1"; // this works with sh, csh, and bash!

  cout << cmd << endl;
  system(cmd.c_str());
  if (!ReadResults(RES_FILE_NAME, sres, error_msg))
    return false;

  string rm_cmd = string("rm -f ") + tmp_fname;
  system(rm_cmd.c_str());

  return true;
}

//---------------------------------------------------------------------------

string ExtractFirstField(const string& s)
{
  istringstream iss(s);

  string sfirst;

  iss >> sfirst;

  return sfirst;
}

//---------------------------------------------------------------------------

bool RunSimulations(double start_time,
		    pair<uint,uint>& sim_counter,
		    const string& cmd, const string& tmp_dir, const int repetitions,
		    const TConfiguration& aggr_conf, 
		    ofstream& fout, 
		    string& error_msg)
{
  int    h, m, s;
  
  for (int i=0; i<repetitions; i++)
    {
      cout << "# simulation " << (++sim_counter.first) << " of " << sim_counter.second;
      if (i != 0)
	cout << ", estimated time to finish " << h << "h " << m << "m " << s << "s";
      cout << endl;

      TSimulationResults sres;
      if (!RunSimulation(cmd, tmp_dir, sres, error_msg))
	return false;

      double current_time = GetCurrentTime();
      TimeToFinish(current_time-start_time, sim_counter.first, sim_counter.second, h, m, s);

      // Print aggragated parameters
      fout << "  ";
      for (uint i=0; i<aggr_conf.size(); i++)
	fout << setw(MATRIX_COLUMN_WIDTH) << ExtractFirstField(aggr_conf[i].second); // this fix the problem with pir
      // fout << setw(MATRIX_COLUMN_WIDTH) << aggr_conf[i].second;

      // Print results;
      fout << setw(MATRIX_COLUMN_WIDTH) << sres.node_coverage
	   << setw(MATRIX_COLUMN_WIDTH) << sres.link_coverage
	   << setw(MATRIX_COLUMN_WIDTH) << sres.nsegments
	   << setw(MATRIX_COLUMN_WIDTH) << sres.avg_seg_length
	   << setw(MATRIX_COLUMN_WIDTH) << sres.latency
	   << endl;
    }

  return true;
}

//---------------------------------------------------------------------------

bool PrintMatlabVariableBegin(const TParametersSpace& aggragated_params_space, 
			      ofstream& fout, string& error_msg)
{
  fout << MATLAB_VAR_NAME << " = [" << endl;
  fout << "% ";
  for (TParametersSpace::const_iterator i=aggragated_params_space.begin();
       i!=aggragated_params_space.end(); i++)
    fout << setw(MATRIX_COLUMN_WIDTH) << i->first;

  fout << setw(MATRIX_COLUMN_WIDTH) << "node_coverage"
       << setw(MATRIX_COLUMN_WIDTH) << "link_coverage"
       << setw(MATRIX_COLUMN_WIDTH) << "nsegments"
       << setw(MATRIX_COLUMN_WIDTH) << "avg_seg_length"
       << setw(MATRIX_COLUMN_WIDTH) << "latency";

  fout << endl;

  return true;
}

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
/*
bool GenMatlabCodeSaturationAnalysis(const string& var_name,
				     ofstream& fout, string& error_msg)
{

  fout << endl 
       << "%-------- Saturation Analysis -----------" << endl
       << "slope=[];"  << endl
       << "for i=2:size(" << var_name << "_throughput,1),"  << endl
       << "    slope(i-1) = (" << var_name << "_throughput(i,2)-" << var_name << "_throughput(i-1,2))/(" << var_name << "_throughput(i,1)-" << var_name << "_throughput(i-1,1));"  << endl
       << "end"  << endl
       << endl
       << "for i=2:size(slope,2),"  << endl
       << "    if slope(i) < (0.95*mean(slope(1:i)))"  << endl
       << "        max_pir = " << var_name << "_throughput(i, 1);"  << endl
       << "        max_throughput = " << var_name << "_throughput(i, 2);"  << endl
       << "        min_delay = " << var_name << "_delay(i, 2);"  << endl
       << "        break;"  << endl
       << "    end"  << endl
       << "end"  << endl;

  return true;
}

//---------------------------------------------------------------------------
*/
bool PrintMatlabVariableEnd(const int repetitions, const int plot_type,
			    ofstream& fout, string& error_msg)
{

    assert(plot_type);

    string xlabel,ylabel,title;
    int fig_no = 1;
    int plot_column;

    switch (plot_type)
    {
	case PLOT_SET1:
	  xlabel = "Node Defect Rate";
	  ylabel = "node_coverage";
	  title = "Node Coverage";
	  plot_column = 1;
	  break;
	case PLOT_SET2:
	  xlabel = "Node Defect Rate";
	  ylabel = "Latency";
	  title = "Impact of Size and Defects on Latency";
	  plot_column = 5;
	  break;
	case PLOT_SET3:
	  xlabel = "Bootstrap Node";
	  ylabel = "node_coverage";
	  title = "Bootstrap Effect";
	  plot_column = 1;
	  break;
    }

    // number of outputs
  int out_col = 5;
  fout << ylabel << " = [];" << endl
       << "for i = 1:rows/" << repetitions << "," << endl
       << "   ifirst = (i - 1) * " << repetitions << " + 1;" << endl
       << "   ilast  = ifirst + " << repetitions << " - 1;" << endl
       << "   tmp = " << MATLAB_VAR_NAME << "(ifirst:ilast, cols-" << out_col <<"+" << plot_column << ");" << endl
       << "   avg = mean(tmp);" << endl
       << "   [h sig ci] = ttest(tmp, 0.1);" << endl
       << "   ci = (ci(2)-ci(1))/2;" << endl
       << "   " << ylabel << " = [" << ylabel << "; " << MATLAB_VAR_NAME << "(ifirst, 1:cols-"<<out_col<<"), avg ci];" << endl
       << "end" << endl
       << endl;


  // coverage is plotted 
  if (plot_type==PLOT_SET1 || plot_type==PLOT_SET3)
  {
      fout << "figure(" << fig_no << ");" << endl
	   << "hold on;" << endl
	   << "title('"<<title<<"')" << endl 
	   << "plot(" << ylabel << "(:,1), " << ylabel << "(:,2), symbol);" << endl
	   << "ylim([0 1])" << endl
	   << "xlabel('"<<xlabel<<"')" << endl
	   << "ylabel('"<<ylabel<<"')" << endl;
  }

  // plotting ideal non defective coverage
  if (plot_type==PLOT_SET1)
  {
	   fout << "x = [0:0.05:0.5]" << endl
	   << "y = 1-x" << endl
	   << "plot(x,y,'--r')" << endl
	   << "legend('DiSR','Ideal')" << endl
	   << endl;
  }

  if (plot_type==PLOT_SET2)
  {
      fout << "figure(" << fig_no << ");" << endl
	   << "hold on;" << endl
	   << "title('"<<title<<"')" << endl 
	   << "plot(" << ylabel << "(:,1), " << ylabel << "(:,2), symbol);" << endl
	   << "xlabel('"<<xlabel<<"')" << endl
	   << "ylabel('"<<ylabel<<"')" << endl;
  }


  return true;
}

//---------------------------------------------------------------------------

bool RunSimulations(const string& script_fname,
	            const TConfigurationSpace& conf_space,
		    const TParameterSpace&     default_params,
		    const TParametersSpace&    aggragated_params_space,
		    const TParameterSpace&     explorer_params,
		    string&                    error_msg)
{
  TExplorerParams eparams;
  if (!ExtractExplorerParams(explorer_params, eparams, error_msg))
    return false;

  // Make dafault parameters string
  string def_cmd_line;
  for (uint i=0; i<default_params.size(); i++)
    def_cmd_line = def_cmd_line + default_params[i] + " ";

  // Explore configuration space
  TConfigurationSpace aggr_conf_space = Explore(aggragated_params_space);

  pair<uint,uint> sim_counter(0, conf_space.size() * aggr_conf_space.size() * eparams.repetitions);
  
  double start_time = GetCurrentTime();
  for (uint i=0; i<conf_space.size(); i++)
    {
      string conf_cmd_line = Configuration2CmdLine(conf_space[i]);

      //string   mfname = Configuration2FunctionName(conf_space[i]);
      string   mfname = script_fname;
      string   fname  = string("out_matlab/")+mfname + ".m";
      ofstream fout;
      if (!PrintHeader(fname, eparams, def_cmd_line, conf_cmd_line, fout, error_msg))
	return false;

      if (!PrintMatlabFunction(mfname, fout, error_msg))
	return false;

      if (!PrintMatlabVariableBegin(aggragated_params_space, fout, error_msg))
	return false;

      for (uint j=0; j<aggr_conf_space.size(); j++)
	{
	  string aggr_cmd_line = Configuration2CmdLine(aggr_conf_space[j]);

	  string cmd = eparams.simulator + " "
            + aggr_cmd_line + " "
	    + def_cmd_line + " "
	    + conf_cmd_line;

	  if (!RunSimulations(start_time,
			      sim_counter, cmd, eparams.tmp_dir, eparams.repetitions,
			      aggr_conf_space[j], fout, error_msg))
	    return false;
	}

      fout << "];" << endl << endl;

      fout << "rows = size(" << MATLAB_VAR_NAME << ", 1);" << endl
	   << "cols = size(" << MATLAB_VAR_NAME << ", 2);" << endl
	   << endl;

      if (!PrintMatlabVariableEnd(eparams.repetitions, eparams.plot_type,fout, error_msg))
	return false;
    }

  return true;
}

//---------------------------------------------------------------------------

bool RunSimulations(const string& script_fname,
		    string&       error_msg)
{
  TParametersSpace ps;

  if (!ParseConfigurationFile(script_fname, ps, error_msg))
    return false;


  TParameterSpace default_params;
  if (!RemoveParameter(ps, DEFAULT_KEY, default_params, error_msg))
    cout << "Warning: " << error_msg << endl;
  

  TParameterSpace  aggregated_params;
  TParametersSpace aggragated_params_space;
  if (!RemoveParameter(ps, AGGREGATION_KEY, aggregated_params, error_msg))
    cout << "Warning: " << error_msg << endl;
  else
    if (!RemoveAggregateParameters(ps, aggregated_params, 
				  aggragated_params_space, error_msg))
      return false;

  TParameterSpace explorer_params;
  if (!RemoveParameter(ps, EXPLORER_KEY, explorer_params, error_msg))
    cout << "Warning: " << error_msg << endl;

  TConfigurationSpace conf_space = Explore(ps);

  if (!RunSimulations(script_fname, conf_space, default_params, 
		      aggragated_params_space, explorer_params, error_msg))
    return false;


  return true;
}


//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Usage: " << argv[0] << " <cfg file> [<cfg file>]" << endl;
      return -1;
    }

  for (int i=1; i<argc; i++)
    {
      string fname(argv[i]);
      cout << "# Exploring configuration space " << fname << endl;

      string error_msg;

      if (!RunSimulations(fname, error_msg))
	cout << "Error: " << error_msg << endl;

      cout << endl;
    }

  return 0;
}

//---------------------------------------------------------------------------
