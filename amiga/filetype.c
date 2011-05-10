/*
 * Copyright 2008, 2011 Chris Young <chris@unsatisfactorysoftware.co.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include "amiga/filetype.h"
#include "amiga/object.h"
#include "content/fetch.h"
#include "content/content.h"
#include "utils/log.h"
#include "utils/utils.h"
#include <proto/icon.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/exec.h>
#include <workbench/icon.h>

/**
 * filetype -- determine the MIME type of a local file
 */

struct MinList *ami_mime_list;

struct ami_mime_entry
{
	lwc_string *mimetype;
	lwc_string *datatype;
	lwc_string *filetype;
	lwc_string *plugincmd;
};

enum
{
	AMI_MIME_MIMETYPE,
	AMI_MIME_DATATYPE,
	AMI_MIME_FILETYPE,
	AMI_MIME_PLUGINCMD
};

const char *fetch_filetype(const char *unix_path)
{
	static char mimetype[50];
	STRPTR ttype = NULL;
	struct DiskObject *dobj = NULL;
	BPTR lock = 0;
	struct DataType *dtn;
	BOOL found = FALSE;

	/* First, check if we appear to have an icon.
	   We'll just do a filename check here for quickness, although the
	   first word ought to be checked against WB_DISKMAGIC really. */

	if(strncmp(unix_path + strlen(unix_path) - 5, ".info", 5) == 0)
	{
		strcpy(mimetype,"image/x-amiga-icon");
		found = TRUE;
	}


	/* Secondly try getting a tooltype "MIMETYPE" and use that as the MIME type.
	    Will fail over to default icons if the file doesn't have a real icon. */

	if(!found)
	{
		if(dobj = GetIconTags(unix_path,ICONGETA_FailIfUnavailable,FALSE,
						TAG_DONE))
		{
			ttype = FindToolType(dobj->do_ToolTypes, "MIMETYPE");
			if(ttype)
			{
				strcpy(mimetype,ttype);
				found = TRUE;
			}

			FreeDiskObject(dobj);
		}
	}

	/* If that didn't work, have a go at guessing it using datatypes.library.  This isn't
		accurate - the base names differ from those used by MIME and it relies on the
		user having a datatype installed which can handle the file. */

	if(!found)
	{
		if (lock = Lock (unix_path, ACCESS_READ))
		{
			if (dtn = ObtainDataTypeA (DTST_FILE, (APTR)lock, NULL))
			{
				ami_datatype_to_mimetype(dtn, &mimetype);
				found = TRUE;
				ReleaseDataType(dtn);
			}
			UnLock(lock);
		}
	}

	/* Have a quick check for file extensions (inc RISC OS filetype).
	 * Makes detection a little more robust, and some of the redirects
	 * caused by links in the SVN tree prevent NetSurf from reading the
	 * MIME type from the icon (step two, above).
	 */

	if((!found) || (strcmp("text/plain", mimetype) == 0))
	{
		if((strncmp(unix_path + strlen(unix_path) - 4, ".css", 4) == 0) ||
			(strncmp(unix_path + strlen(unix_path) - 4, ",f79", 4) == 0))
		{
			strcpy(mimetype,"text/css");
			found = TRUE;
		}

		if((strncmp(unix_path + strlen(unix_path) - 4, ".htm", 4) == 0) ||
			(strncmp(unix_path + strlen(unix_path) - 5, ".html", 5) == 0) ||
			(strncmp(unix_path + strlen(unix_path) - 4, ",faf", 4) == 0))
		{
			strcpy(mimetype,"text/html");
			found = TRUE;
		}
	}

	if(!found) strcpy(mimetype,"text/plain"); /* If all else fails */

	return mimetype;
}


char *fetch_mimetype(const char *ro_path)
{
	return strdup(fetch_filetype(ro_path));
}

const char *ami_content_type_to_file_type(content_type type)
{
	switch(type)
	{
		case CONTENT_HTML:
			return "html";
		break;

		case CONTENT_TEXTPLAIN:
			return "ascii";
		break;

		case CONTENT_CSS:
			return "css";
		break;

		default:
			return "project";	
		break;
	}
}

void ami_datatype_to_mimetype(struct DataType *dtn, char *mimetype)
{
	struct DataTypeHeader *dth = dtn->dtn_Header;

	switch(dth->dth_GroupID)
	{
		case GID_TEXT:
		case GID_DOCUMENT:
			if(strcmp("ascii",dth->dth_BaseName)==0)
			{
				strcpy(mimetype,"text/plain");
			}
			else if(strcmp("simplehtml",dth->dth_BaseName)==0)
			{
				strcpy(mimetype,"text/html");
			}
			else
			{
				sprintf(mimetype,"text/%s",dth->dth_BaseName);
			}
		break;
		case GID_SOUND:
		case GID_INSTRUMENT:
		case GID_MUSIC:
			sprintf(mimetype,"audio/%s",dth->dth_BaseName);
		break;
		case GID_PICTURE:
			sprintf(mimetype,"image/%s",dth->dth_BaseName);
			if(strcmp("sprite",dth->dth_BaseName)==0)
			{
				strcpy(mimetype,"image/x-riscos-sprite");
			}
			if(strcmp("mng",dth->dth_BaseName)==0)
			{
				strcpy(mimetype,"video/mng");
			}
		break;
		case GID_ANIMATION:
		case GID_MOVIE:
			sprintf(mimetype,"video/%s",dth->dth_BaseName);
		break;
		case GID_SYSTEM:
		default:
			if(strcmp("directory",dth->dth_BaseName)==0)
			{
				strcpy(mimetype,"application/x-netsurf-directory");
			}
			else if(strcmp("binary",dth->dth_BaseName)==0)
			{
				strcpy(mimetype,"application/octet-stream");
			}
			else sprintf(mimetype,"application/%s",dth->dth_BaseName);
		break;
	}
}

nserror ami_mime_init(const char *mimefile)
{
	lwc_string *type;
	lwc_error lerror;
	nserror error;
	char buffer[256];
	BPTR fh = 0;
	struct RDArgs *rargs = NULL;
	STRPTR template = "MIMETYPE/A,DT=DATATYPE/K,TYPE=DEFICON/K,CMD=PLUGINCMD/K";
	long rarray[] = {0,0,0,0};
	struct nsObject *node;
	struct ami_mime_entry *mimeentry;

	ami_mime_list = NewObjList();

	rargs = AllocDosObjectTags(DOS_RDARGS,TAG_DONE);

	if(fh = FOpen(mimefile, MODE_OLDFILE, 0))
	{
		while(FGets(fh, (UBYTE *)&buffer, 256) != 0)
		{
			rargs->RDA_Source.CS_Buffer = (char *)&buffer;
			rargs->RDA_Source.CS_Length = 256;
			rargs->RDA_Source.CS_CurChr = 0;

			rargs->RDA_DAList = NULL;
			rargs->RDA_Buffer = NULL;
			rargs->RDA_BufSiz = 0;
			rargs->RDA_ExtHelp = NULL;
			rargs->RDA_Flags = 0;

			rarray[AMI_MIME_MIMETYPE] = 0;
			rarray[AMI_MIME_DATATYPE] = 0;
			rarray[AMI_MIME_FILETYPE] = 0;
			rarray[AMI_MIME_PLUGINCMD] = 0;

			if(ReadArgs(template, rarray, rargs))
			{
				node = AddObject(ami_mime_list, AMINS_MIME);
				mimeentry = AllocVec(sizeof(struct ami_mime_entry), MEMF_PRIVATE | MEMF_CLEAR);
				node->objstruct = mimeentry;

				if(rarray[AMI_MIME_MIMETYPE])
				{
					lerror = lwc_intern_string((char *)rarray[AMI_MIME_MIMETYPE],
								strlen((char *)rarray[AMI_MIME_MIMETYPE]), &mimeentry->mimetype);
					if (lerror != lwc_error_ok)
						return NSERROR_NOMEM;
				}

				if(rarray[AMI_MIME_DATATYPE])
				{
					lerror = lwc_intern_string((char *)rarray[AMI_MIME_DATATYPE],
								strlen((char *)rarray[AMI_MIME_DATATYPE]), &mimeentry->datatype);
					if (lerror != lwc_error_ok)
						return NSERROR_NOMEM;
				}

				if(rarray[AMI_MIME_FILETYPE])
				{
					lerror = lwc_intern_string((char *)rarray[AMI_MIME_FILETYPE],
								strlen((char *)rarray[AMI_MIME_FILETYPE]), &mimeentry->filetype);
					if (lerror != lwc_error_ok)
						return NSERROR_NOMEM;
				}

				if(rarray[AMI_MIME_PLUGINCMD])
				{
					lerror = lwc_intern_string((char *)rarray[AMI_MIME_PLUGINCMD],
								strlen((char *)rarray[AMI_MIME_PLUGINCMD]), &mimeentry->plugincmd);
					if (lerror != lwc_error_ok)
						return NSERROR_NOMEM;
				}
			}
		}
		FClose(fh);
	}
}

void ami_mime_free(void)
{
	FreeObjList(ami_mime_list);
}

void ami_mime_entry_free(struct ami_mime_entry *mimeentry)
{
	if(mimeentry->mimetype) lwc_string_unref(mimeentry->mimetype);
	if(mimeentry->datatype) lwc_string_unref(mimeentry->datatype);
	if(mimeentry->filetype) lwc_string_unref(mimeentry->filetype);
	if(mimeentry->plugincmd) lwc_string_unref(mimeentry->plugincmd);
}


/**
 * Return next matching MIME entry
 *
 * \param search lwc_string to search for (or NULL for all)
 * \param type of value being searched for (AMI_MIME_#?)
 * \param start_node node to continue search (updated on exit)
 * \return entry or NULL if no match
 */

struct ami_mime_entry *ami_mime_entry_locate(lwc_string *search,
		int type, struct Node **start_node)
{
	struct nsObject *node;
	struct nsObject *nnode;
	struct ami_mime_entry *mimeentry;
	lwc_error lerror;
	bool ret = false;

	if(IsMinListEmpty(ami_mime_list)) return NULL;

	if(*start_node)
	{
		node = (struct nsObject *)GetSucc(*start_node);
		if(node == NULL) return NULL;
	}
	else
	{
		node = (struct nsObject *)GetHead((struct List *)ami_mime_list);
	}

	do
	{
		nnode=(struct nsObject *)GetSucc((struct Node *)node);
		mimeentry = node->objstruct;

		lerror = lwc_error_ok;

		switch(type)
		{
			case AMI_MIME_MIMETYPE:
				if(search != NULL)
					lerror = lwc_string_isequal(mimeentry->mimetype, search, &ret);
				else if(mimeentry->mimetype != NULL)
					ret = true;
			break;

			case AMI_MIME_DATATYPE:
				if(search != NULL)
					lerror = lwc_string_isequal(mimeentry->datatype, search, &ret);
				else if(mimeentry->datatype != NULL)
					ret = true;
			break;

			case AMI_MIME_FILETYPE:
				if(search != NULL)
					lerror = lwc_string_isequal(mimeentry->filetype, search, &ret);
				else if(mimeentry->filetype != NULL)
					ret = true;
			break;

			case AMI_MIME_PLUGINCMD:
				if(search != NULL)
					lerror = lwc_string_isequal(mimeentry->plugincmd, search, &ret);
				else if(mimeentry->plugincmd != NULL)
					ret = true;
			break;
		}

		if((lerror == lwc_error_ok) && (ret == true))
			break;

	}while(node=nnode);

	*start_node = (struct Node *)node;

	if(ret == true) return mimeentry;
		else return NULL;
}

/**
 * Return a MIME Type matching a DataType
 *
 * \param dt a DataType structure
 * \param mimetype lwc_string to hold the MIME type
 * \param start_node node to feed back in to continue search
 * \return node or NULL if no match
 */

struct Node *ami_mime_from_datatype(struct DataType *dt,
		lwc_string **mimetype, struct Node *start_node)
{
	struct DataTypeHeader *dth;
	struct Node *node;
	struct ami_mime_entry *mimeentry;
	lwc_string *dt_name;
	lwc_error lerror;

	if(dt == NULL) return NULL;

	dth = dt->dtn_Header;
	lerror = lwc_intern_string(dth->dth_Name, strlen(dth->dth_Name), &dt_name);
	if (lerror != lwc_error_ok)
		return NULL;

	node = start_node;
	mimeentry = ami_mime_entry_locate(dt_name, AMI_MIME_DATATYPE, &node);

	lwc_string_unref(dt_name);

	if(mimeentry != NULL)
	{
		*mimetype = mimeentry->mimetype;
		return (struct Node *)node;
	}
	else
	{
		return NULL;
	}
}

/**
 * Return the DefIcons type matching a MIME type
 *
 * \param mimetype lwc_string MIME type
 * \param filetype ptr to lwc_string to hold DefIcons type
 * \param start_node node to feed back in to continue search
 * \return node or NULL if no match
 */

struct Node *ami_mime_to_filetype(lwc_string *mimetype,
		lwc_string **filetype, struct Node *start_node)
{
	struct Node *node;
	struct ami_mime_entry *mimeentry;

	node = start_node;
	mimeentry = ami_mime_entry_locate(mimetype, AMI_MIME_MIMETYPE, &node);

	if(mimeentry != NULL)
	{
		*filetype = mimeentry->filetype;
		return (struct Node *)node;
	}
	else
	{
		return NULL;
	}
}

/**
 * Return all MIME types containing a plugincmd
 *
 * \param mimetype ptr to lwc_string MIME type
 * \param start_node node to feed back in to continue search
 * \return node or NULL if no match
 */

struct Node *ami_mime_has_cmd(lwc_string **mimetype, struct Node *start_node)
{
	struct Node *node;
	struct ami_mime_entry *mimeentry;

	node = start_node;
	mimeentry = ami_mime_entry_locate(NULL, AMI_MIME_PLUGINCMD, &node);

	if(mimeentry != NULL)
	{
		*mimetype = mimeentry->mimetype;
		return (struct Node *)node;
	}
	else
	{
		return NULL;
	}
}

/**
 * Return the plugincmd matching a MIME type
 *
 * \param mimetype lwc_string MIME type
 * \param plugincmd ptr to lwc_string to hold plugincmd
 * \param start_node node to feed back in to continue search
 * \return node or NULL if no match
 */

struct Node *ami_mime_to_plugincmd(lwc_string *mimetype,
		lwc_string **plugincmd, struct Node *start_node)
{
	struct Node *node;
	struct ami_mime_entry *mimeentry;

	node = start_node;
	mimeentry = ami_mime_entry_locate(mimetype, AMI_MIME_MIMETYPE, &node);

	if(mimeentry != NULL)
	{
		*plugincmd = mimeentry->plugincmd;
		return (struct Node *)node;
	}
	else
	{
		return NULL;
	}
}

/**
 * Compare the MIME type of an hlcache_handle to a DefIcons type
 */

bool ami_mime_compare(struct hlcache_handle *c, const char *type)
{
	bool ret = false;
	lwc_error lerror;
	lwc_string *filetype;
	lwc_string *mime = content_get_mime_type(c);

	lerror = lwc_intern_string(type, strlen(type), &filetype);
	if (lerror != lwc_error_ok)
		return false;

	lerror = lwc_string_isequal(filetype, mime, &ret);
	if (lerror != lwc_error_ok)
		return false;

	lwc_string_unref(filetype);

	return ret;
}
