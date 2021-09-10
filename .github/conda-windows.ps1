if (-not (Test-Path C:\conda_envs -PathType Container)) 
{
  conda create --yes -p C:\conda_envs\cpp_pkgs -c conda-forge python=3.9
  conda activate C:\conda_envs\cpp_pkgs
  conda install --yes -c conda-forge boost-cpp eigen nlohmann_json mkl mkl-devel mkl-include catch2
  
  conda create --yes -p C:\conda_envs\cp37-cp37m -c conda-forge python=3.7
  conda activate C:\conda_envs\cp37-cp37m
  pip install --no-cache-dir -r dev-requirements.txt
  pip install --no-cache-dir twine
  
  conda create --yes -p C:\conda_envs\cp38-cp38 -c conda-forge python=3.8
  conda activate C:\conda_envs\cp38-cp38
  pip install --no-cache-dir -r dev-requirements.txt
  pip install --no-cache-dir twine
  
  conda create --yes -p C:\conda_envs\cp39-cp39 -c conda-forge python=3.9
  conda activate C:\conda_envs\cp39-cp39
  pip install --no-cache-dir -r dev-requirements.txt
  pip install --no-cache-dir twine
}
