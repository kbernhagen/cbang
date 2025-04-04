/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Tar.h"

#include <cbang/Exception.h>

#include <algorithm>

using namespace std;
using namespace cb;


const char Tar::zero_block[512] = {0, };


namespace {
  streamsize compute_padding(streamsize size) {
    return (size & 511) ? 512 - (size & 511) : 0;
  }
}



Tar::Tar(unsigned bufferSize) :
  bufferSize(bufferSize), buf(new char[bufferSize]) {}


unsigned Tar::writeFile(const string &filename, ostream &dst, istream &src,
                        uint32_t mode) {
  // Get stream size
  src.seekg(0, ios::end);
  streampos size = src.tellg();
  src.seekg(0, ios::beg);

  // Write header
  setFilename(filename);
  setSize(size);
  setType(TarHeader::NORMAL_FILE);
  setMode(mode);
  writeHeader(dst);

  // Write data
  return writeFileData(dst, src, (streamsize)size);
}


unsigned Tar::writeFile(const string &filename, ostream &dst, const char *data,
                        streamsize size, uint32_t mode) {
  // Write header
  setFilename(filename);
  setSize(size);
  setType(TarHeader::NORMAL_FILE);
  setMode(mode);
  writeHeader(dst);

  // Write data
  return writeFileData(dst, data, size);
}


unsigned Tar::writeFileData(std::ostream &dst, std::istream &src,
                            streamsize size) {
  streamsize n;

  while (true) {
    src.read(buf.get(), bufferSize);
    n = src.gcount();
    if (n) dst.write(buf.get(), n);
    else break;
  }

  // Pad with zeros
  streamsize padding = compute_padding(size);
  if (padding) dst.write(zero_block, padding);

  return size + padding;
}


unsigned Tar::writeFileData(std::ostream &dst, const char *data,
                            streamsize size) {
  dst.write(data, size);

  // Pad with zeros
  streamsize padding = compute_padding(size);
  if (padding) dst.write(zero_block, padding);

  return size + padding;
}


unsigned Tar::writeDir(const string &name, ostream &dst, uint32_t mode) {
  if (!name.length() || name[name.length() - 1] != '/')
    setFilename(name + '/');
  else setFilename(name);

  setSize(0);
  setType(TarHeader::DIRECTORY);
  setMode(mode);
  writeHeader(dst);

  return 512;
}


unsigned Tar::writeFooter(ostream &dst) {
  dst.write(zero_block, 512);
  dst.write(zero_block, 512);

  return 1024;
}


void Tar::readFile(ostream &dst, istream &src) {
  streamsize size = getSize();
  streamsize padding = compute_padding(size);
  streamsize n;

  while (size) {
    unsigned bytes = min((unsigned)size, bufferSize);
    src.read(buf.get(), bytes);
    if (!(n = src.gcount()))
      THROW("Error reading " << bytes << " bytes from tar file");

    size -= n;

    dst.write(buf.get(), n);
    if (dst.fail()) THROW(string("Failed to write '") + getFilename() + "'");
  }

  src.ignore(padding);
}


void Tar::skipFile(istream &src) {
  src.ignore(getSize() + compute_padding(getSize()));
}
