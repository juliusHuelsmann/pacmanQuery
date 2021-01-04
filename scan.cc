#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr auto numThreads = 4;

struct Runner {

  struct Map {
    std::unordered_map<std::string, std::unique_ptr<Map>> kids;
    std::string content;
  };

  std::string const rootDir;

  explicit Runner(std::string const &root) : rootDir(root) {
    threads.reserve(numThreads);
    namespace fs = std::filesystem;

    std::ios_base::sync_with_stdio(false);
    std::array<std::pair<std::vector<std::string>, std::vector<std::string>>, numThreads> cmds{};
    std::size_t ncmd = 0;

    for (const auto &entry : fs::recursive_directory_iterator(rootDir)) {
      ++amount;
      auto const path = entry.path().string();
      auto const cmd = (std::string("pacman -Qo ") + path +
                        std::string(">>/dev/null 2>>/dev/null")); // 0 gut,
      cmds.at(ncmd).first.emplace_back(std::move(cmd));
      cmds.at(ncmd).second.emplace_back(std::move(path));

      if (++ncmd == numThreads) ncmd = 0;
      if (amount > 1000) break; // XXX
    }
    std::cout << "starting" << std::endl;

    for (auto const &cmd : cmds) {
      threads.emplace_back(&Runner::insertMultiple, std::move(cmd.first),
                           std::move(cmd.second), rootDir, &mutex, &good, &bad);
    }

    for (auto &t : threads) { t.join(); }
    threads.clear();
    std::cout << amount << " files processed." << std::endl;

  }
  
  void printMap(Map const *g, std::string path) {
    for (auto const &[key, value] : g->kids) {
      if (value->kids.empty()) {
        std::cout << path << "/" << key << ":" << value->content << "\n";
      }
      printMap(value.get(), path + std::string("/") + key);
    }
  }

  void traverse(Map const *g, Map const *b, std::string path) {
    for (auto const &[key, value] : b->kids) {
      if (g->kids.count(key)) {
        //std::cout << "traverse" << key << "\n";
        traverse(g->kids.at(key).get(), value.get(), path + std::string("/") + key);
      } else {
        if (g->content.empty()) 
          std::cout << "Complete path unused: " << path << "\n";
        else
          std::cout << "Unused file:          " << value->content << " at path " << path << '/' << key << std::endl;
      }
    }
  }

  ~Runner() {}

  static void insertToTree(std::string &&name, std::string const &root, Map *tree) {
    
    std::stringstream ss(name.substr(root.size()));
    for (std::string col; getline(ss, col, '/');) {
      auto const contains = tree->kids.count(col);
      //std::cout << col << contains << std::endl;
      //auto const isLeaf = !tree->content.empty();

      if (!contains) { tree->kids.emplace(col, std::make_unique<Map>()); }
      tree = tree->kids[col].get();
    }

    tree->content = name;
    //std::cout << (name) << std::endl; 
  }

  static void insert(std::string &&command, std::string &&name,
                     std::string const &root, std::mutex *mut, Map *good,
                     Map *bad) {
    auto const ex = WEXITSTATUS(std::system(command.c_str()));

    std::lock_guard<std::mutex> const lock{*mut};
    insertToTree(std::move(name), root, ex ? bad : good);
  }

  static void insertMultiple(std::vector<std::string> &&commands,
                             std::vector<std::string> &&names,
                             std::string const &root, std::mutex *mut,
                             Map *good, Map *bad) {

    assert(commands.size() == names.size());
    for (std::size_t s = 0; s < commands.size(); ++s) {

      auto const ex = WEXITSTATUS(std::system(commands.at(s).c_str()));
      std::lock_guard<std::mutex> const lock{*mut};
      insertToTree(std::move(names.at(s)), root, ex ? bad : good);
    }
  }

  static void insertMultipleOneCall(std::string &&command,
                                    std::vector<std::string> &&names,
                                    std::string const &root, std::mutex *mut,
                                    Map *good, Map *bad) {
    // XXX

    //auto const ex = WEXITSTATUS(std::system(commands.at(s).c_str()));
    //std::lock_guard<std::mutex> const lock{*mut};
    //insertToTree(std::move(names.at(s)), root, ex ? bad : good);
  }

  std::size_t amount{0};
  std::vector<std::thread> threads{};
  Map bad, good;
  std::mutex mutex;
};

int main(int argc, char **argv) {
  assert(argc == 1 || argc == 2);
  std::string const rootDir = argc == 1 ? "/usr/local/" : argv[1];
  std::cout << rootDir << std::endl;
  Runner r{std::move(rootDir)};
  std::cout << "\n\ngood" << std::endl;
  r.printMap(&(r.good), r.rootDir);
  std::cout << "\n\nbad" << std::endl;
  r.printMap(&(r.bad), r.rootDir);
  std::cout << "\n\nboth" << std::endl;
  r.traverse(&(r.good), &(r.bad), r.rootDir);

  return 0;
}
