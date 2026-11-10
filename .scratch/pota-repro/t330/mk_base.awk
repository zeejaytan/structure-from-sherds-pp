# mk_base.awk -- insert the g_len_scale hook at the top of main() in the shipped batch shim.
# Output differs from the shipped file by exactly three inserted statements; nothing else changes.
{ if (flag == 1 && $0 == "{") {
    print
    print "\textern double g_len_scale;"
    print "\tif (argc > 2 && argv[2] != NULL) { g_len_scale = std::atof(argv[2]); }"
    print "\tcout << \"T330_S g_len_scale=\" << g_len_scale << endl;"
    flag = 0
    next
  }
  print
  if ($0 ~ /^int main\(int argc, char\*\* argv\)$/) flag = 1
}
