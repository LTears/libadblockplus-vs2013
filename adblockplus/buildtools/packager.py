# coding: utf-8

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Note: These are the base functions common to all packagers, the actual
# packagers are implemented in packagerGecko and packagerChrome.

import sys, os, re, codecs, subprocess, json, zipfile
from StringIO import StringIO
from chainedconfigparser import ChainedConfigParser

import buildtools

def getDefaultFileName(metadata, version, ext):
  return '%s-%s.%s' % (metadata.get('general', 'basename'), version, ext)

def getMetadataPath(baseDir, type):
  return os.path.join(baseDir, 'metadata.%s' % type)

def getDevEnvPath(baseDir):
  return os.path.join(baseDir, 'devenv')

def readMetadata(baseDir, type):
  parser = ChainedConfigParser()
  parser.optionxform = lambda option: option
  parser.read(getMetadataPath(baseDir, type))
  return parser

def getBuildNum(baseDir):
  try:
    from buildtools.ensure_dependencies import Mercurial, Git
    if Mercurial().istype(baseDir):
      result = subprocess.check_output(['hg', 'id', '-R', baseDir, '-n'])
      return re.sub(r'\D', '', result)
    elif Git().istype(baseDir):
      result = subprocess.check_output(['git', 'rev-list', 'HEAD'], cwd=baseDir)
      return len(result.splitlines())
  except subprocess.CalledProcessError:
    pass

  return '0'

def getBuildVersion(baseDir, metadata, releaseBuild, buildNum=None):
  version = metadata.get('general', 'version')
  if not releaseBuild:
    if buildNum == None:
      buildNum = getBuildNum(baseDir)
    buildNum = str(buildNum)
    if len(buildNum) > 0:
      if re.search(r'(^|\.)\d+$', version):
        # Numerical version number - need to fill up with zeros to have three
        # version components.
        while version.count('.') < 2:
          version += '.0'
      version += '.' + buildNum
  return version

def getTemplate(template, autoEscape=False):
  import jinja2

  templatePath = buildtools.__path__[0]
  if autoEscape:
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(templatePath), autoescape=True)
  else:
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(templatePath))
  env.filters.update({'json': json.dumps})
  return env.get_template(template)

class Files(dict):
  def __init__(self, includedFiles, ignoredFiles, process=None):
    self.includedFiles = includedFiles
    self.ignoredFiles = ignoredFiles
    self.process = process

  def __setitem__(self, key, value):
    if self.process:
      value = self.process(key, value)
    dict.__setitem__(self, key, value)

  def isIncluded(self, relpath):
    parts = relpath.split('/')
    if not parts[0] in self.includedFiles:
      return False
    for part in parts:
      if part in self.ignoredFiles:
        return False
    return True

  def read(self, path, relpath='', skip=None):
    if os.path.isdir(path):
      for file in os.listdir(path):
        name = relpath + ('/' if relpath != '' else '') + file
        if (skip == None or file not in skip) and self.isIncluded(name):
          self.read(os.path.join(path, file), name)
    else:
      with open(path, 'rb') as file:
        if relpath in self:
          print >>sys.stderr, 'Warning: File %s defined multiple times' % relpath
        else:
          self[relpath] = file.read()

  def readMappedFiles(self, mappings):
    for item in mappings:
      target, source = item

      # Make sure the file is inside an included directory
      if '/' in target and not self.isIncluded(target):
        continue
      parts = source.split('/')
      path = os.path.join(os.path.dirname(item.source), *parts)
      if os.path.exists(path):
        self.read(path, target)
      else:
        print >>sys.stderr, 'Warning: Mapped file %s doesn\'t exist' % source

  def preprocess(self, filenames, params={}):
    import jinja2
    env = jinja2.Environment()

    for filename in filenames:
      env.autoescape = os.path.splitext(filename)[1].lower() in ('.html', '.xml')
      template = env.from_string(self[filename].decode('utf-8'))
      self[filename] = template.render(params).encode('utf-8')

  def zip(self, outFile, sortKey=None):
    zip = zipfile.ZipFile(outFile, 'w', zipfile.ZIP_DEFLATED)
    names = self.keys()
    names.sort(key=sortKey)
    for name in names:
      zip.writestr(name, self[name])
    zip.close()

  def zipToString(self, sortKey=None):
    buffer = StringIO()
    self.zip(buffer, sortKey=sortKey)
    return buffer.getvalue()
