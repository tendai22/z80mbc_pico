# z80mbc_pico: 開発記録

## CLI 開発環境

Getting Started ... の Appendix C に説明がある。

pico_setup.sh を実行すればツールチェーン、sdk、examples 一式インストールしてくれる。

Raspberry Pi 上で実行することになっているが、WSL-Ubuntsu で普通に環境構築してくれた。

```
$ wget https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh 
$ chmod +x pico_setup.sh
$ ./pico_setup.sh
```

`PICO_SDK_PATH` の定義が必要。`$HOME/.pico-sdk/sdk/2.1.1` にインストールしてくれていた。
`~/.profile` に書いておきたい。



## コマンドラインでビルド

git clone のあと、`mkdir build`, `cd build` してからそこで `cmake ..` で良い。`build` 直下にファイルが生成され、`make` コマンドで実行可能ファイル一式ビルドしてくれる。

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

`z80mbc_pico` 直下で `cmake .` すると、ファイルあれこれ一杯散らかしてくれる。

