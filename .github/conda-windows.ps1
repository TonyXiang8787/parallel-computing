if (-not (Test-Path C:\conda_envs -PathType Container)) 
{
  $env:CONDA\bin\conda create --yes -p C:\conda_envs\cpp_pkgs -c conda-forge python=3.9
  $env:CONDA\bin\conda activate cpp_pkgs
  conda install --yes -c conda-forge boost-cpp eigen nlohmann_json mkl mkl-devel
}
