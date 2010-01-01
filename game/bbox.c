//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file bbox.c
/// @brief
/// @details

#include "bbox.h"

#include "egoboo_math.h"
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static Uint32    cv_list_count = 0;
static OVolume_t cv_list[1000];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_cv_point_data
{
  bool_t  inside;
  fvec3_t   pos;
  float   rads;
};
typedef struct s_cv_point_data cv_point_data_t;

static int cv_point_data_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
EGO_CONST aabb_lst_t * aabb_lst_ctor(aabb_lst_t * lst)
{
  if(NULL == lst) return NULL;
  memset(lst, 0, sizeof(aabb_lst_t));
  return lst;
}

//--------------------------------------------------------------------------------------------
EGO_CONST aabb_lst_t * aabb_lst_dtor(aabb_lst_t * lst)
{
  if(NULL == lst) return NULL;

  if( lst->count > 0 )
  {
    EGOBOO_DELETE(lst->list);
  }

  lst->count = 0;
  lst->list  = NULL;

  return lst;
}

//--------------------------------------------------------------------------------------------
EGO_CONST aabb_lst_t * aabb_lst_renew(aabb_lst_t * lst)
{
  if(NULL == lst) return NULL;

  aabb_lst_dtor(lst);
  return aabb_lst_ctor(lst);
}

//--------------------------------------------------------------------------------------------
EGO_CONST aabb_lst_t * aabb_lst_alloc(aabb_lst_t * lst, int count)
{
  if(NULL == lst) return NULL;

  aabb_lst_dtor(lst);

  if(count>0)
  {
    lst->list = EGOBOO_NEW_ARY( ego_aabb_t, count );
    if(NULL != lst->list)
    {
      lst->count = count;
    }
  }

  return lst;
}

//--------------------------------------------------------------------------------------------
EGO_CONST aabb_lst_t * aabb_lst_realloc(aabb_lst_t * lst, int count)
{
  // check for bad list
  if(NULL == lst) return NULL;

  // check for no change in the count
  if(count == lst->count) return lst;

  // check another dumb case
  if(count==0)
  {
    return aabb_lst_dtor(lst);
  }

  lst->list = (ego_aabb_t *)realloc(lst->list, count * sizeof(ego_aabb_t));
  if(NULL == lst->list)
  {
    lst->count = 0;
  }
  else
  {
    lst->count = count;
  }

  return lst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE EGO_CONST aabb_ary_t * bbox_ary_ctor(aabb_ary_t * ary)
{
  if(NULL == ary) return NULL;

  memset(ary, 0, sizeof(aabb_ary_t));

  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST aabb_ary_t * bbox_ary_dtor(aabb_ary_t * ary)
{
  int i;

  if(NULL == ary) return NULL;

  if(NULL !=ary->list)
  {
    for(i=0; i<ary->count; i++)
    {
      aabb_lst_dtor(ary->list + i);
    }

    EGOBOO_DELETE(ary->list);
  }

  ary->count = 0;
  ary->list = NULL;

  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST aabb_ary_t * bbox_ary_renew(aabb_ary_t * ary)
{
  if(NULL == ary) return NULL;
  bbox_ary_dtor(ary);
  return bbox_ary_ctor(ary);
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST aabb_ary_t * bbox_ary_alloc(aabb_ary_t * ary, int count)
{
  if(NULL == ary) return NULL;

  bbox_ary_dtor(ary);

  if(count>0)
  {
    ary->list = EGOBOO_NEW_ARY( aabb_lst_t, count );
    if(NULL != ary->list)
    {
      ary->count = count;
    }
  }

  return ary;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
OVolume_t OVolume_merge(OVolume_t * pv1, OVolume_t * pv2)
{
  OVolume_t rv;

  rv.lod = -1;

  if(NULL ==pv1 && NULL ==pv2)
  {
    return rv;
  }
  else if(NULL ==pv2)
  {
    return *pv1;
  }
  else if(NULL ==pv1)
  {
    return *pv2;
  }
  else
  {
    bool_t binvalid;

    // check for uninitialized volumes
    if(-1==pv1->lod && -1==pv2->lod)
    {
      return rv;
    }
    else if(-1==pv1->lod)
    {
      return *pv2;
    }
    else if(-1==pv2->lod)
    {
      return *pv1;
    };

    // merge the volumes

    rv.oct.mins[OCT_X] = MIN(pv1->oct.mins[OCT_X], pv2->oct.mins[OCT_X]);
    rv.oct.maxs[OCT_X] = MAX(pv1->oct.maxs[OCT_X], pv2->oct.maxs[OCT_X]);

    rv.oct.mins[OCT_Y] = MIN(pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_Y]);
    rv.oct.maxs[OCT_Y] = MAX(pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_Y]);

    rv.oct.mins[OCT_Z] = MIN(pv1->oct.mins[OCT_Z], pv2->oct.mins[OCT_Z]);
    rv.oct.maxs[OCT_Z] = MAX(pv1->oct.maxs[OCT_Z], pv2->oct.maxs[OCT_Z]);

    rv.oct.mins[OCT_XY] = MIN(pv1->oct.mins[OCT_XY], pv2->oct.mins[OCT_XY]);
    rv.oct.maxs[OCT_XY] = MAX(pv1->oct.maxs[OCT_XY], pv2->oct.maxs[OCT_XY]);

    rv.oct.mins[OCT_YX] = MIN(pv1->oct.mins[OCT_YX], pv2->oct.mins[OCT_YX]);
    rv.oct.maxs[OCT_YX] = MAX(pv1->oct.maxs[OCT_YX], pv2->oct.maxs[OCT_YX]);

    // check for an invalid volume
    binvalid = (rv.oct.mins[OCT_X] >= rv.oct.maxs[OCT_X]) || (rv.oct.mins[OCT_Y] >= rv.oct.maxs[OCT_Y]) || (rv.oct.mins[OCT_Z] >= rv.oct.maxs[OCT_Z]);
    binvalid = binvalid ||(rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY]) || (rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX]);

    rv.lod = binvalid ? -1 : 1;
  }

  return rv;
}

//--------------------------------------------------------------------------------------------
OVolume_t OVolume_intersect(OVolume_t * pv1, OVolume_t * pv2)
{
  OVolume_t rv;

  rv.lod = -1;

  if(NULL == pv1 || NULL == pv2)
  {
    return rv;
  }
  else
  {
    // check for uninitialized volumes
    if(-1==pv1->lod && -1==pv2->lod)
    {
      return rv;
    }
    else if(-1==pv1->lod)
    {
      return *pv2;
    }
    else if (-1==pv2->lod)
    {
      return *pv1;
    }

    // intersect the volumes
    rv.oct.mins[OCT_X] = MAX(pv1->oct.mins[OCT_X], pv2->oct.mins[OCT_X]);
    rv.oct.maxs[OCT_X] = MIN(pv1->oct.maxs[OCT_X], pv2->oct.maxs[OCT_X]);
    if(rv.oct.mins[OCT_X] >= rv.oct.maxs[OCT_X]) return rv;

    rv.oct.mins[OCT_Y] = MAX(pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_Y]);
    rv.oct.maxs[OCT_Y] = MIN(pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_Y]);
    if(rv.oct.mins[OCT_Y] >= rv.oct.maxs[OCT_Y]) return rv;

    rv.oct.mins[OCT_Z] = MAX(pv1->oct.mins[OCT_Z], pv2->oct.mins[OCT_Z]);
    rv.oct.maxs[OCT_Z] = MIN(pv1->oct.maxs[OCT_Z], pv2->oct.maxs[OCT_Z]);
    if(rv.oct.mins[OCT_Z] >= rv.oct.maxs[OCT_Z]) return rv;

    if(pv1->lod >= 0 && pv2->lod >= 0)
    {
      rv.oct.mins[OCT_XY] = MAX(pv1->oct.mins[OCT_XY], pv2->oct.mins[OCT_XY]);
      rv.oct.maxs[OCT_XY] = MIN(pv1->oct.maxs[OCT_XY], pv2->oct.maxs[OCT_XY]);
      if(rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY]) return rv;

      rv.oct.mins[OCT_YX] = MAX(pv1->oct.mins[OCT_YX], pv2->oct.mins[OCT_YX]);
      rv.oct.maxs[OCT_YX] = MIN(pv1->oct.maxs[OCT_YX], pv2->oct.maxs[OCT_YX]);
      if(rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX]) return rv;
    }
    else if(pv1->lod >= 0)
    {
      rv.oct.mins[OCT_XY] = MAX(pv1->oct.mins[OCT_XY], pv2->oct.mins[OCT_X] + pv2->oct.mins[OCT_Y]);
      rv.oct.maxs[OCT_XY] = MIN(pv1->oct.maxs[OCT_XY], pv2->oct.maxs[OCT_X] + pv2->oct.maxs[OCT_Y]);
      if(rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY]) return rv;

      rv.oct.mins[OCT_YX] = MAX(pv1->oct.mins[OCT_YX], -pv2->oct.maxs[OCT_X] + pv2->oct.mins[OCT_Y]);
      rv.oct.maxs[OCT_YX] = MIN(pv1->oct.maxs[OCT_YX], -pv2->oct.mins[OCT_X] + pv2->oct.maxs[OCT_Y]);
      if(rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX]) return rv;
    }
    else if(pv2->lod >= 0)
    {
      rv.oct.mins[OCT_XY] = MAX(pv1->oct.mins[OCT_X] + pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_XY]);
      rv.oct.maxs[OCT_XY] = MIN(pv1->oct.maxs[OCT_X] + pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_XY]);
      if(rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY]) return rv;

      rv.oct.mins[OCT_YX] = MAX(-pv1->oct.maxs[OCT_X] + pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_YX]);
      rv.oct.maxs[OCT_YX] = MIN(-pv1->oct.mins[OCT_X] + pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_YX]);
      if(rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX]) return rv;
    }
    else
    {
      rv.oct.mins[OCT_XY] = MAX(pv1->oct.mins[OCT_X] + pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_X] + pv2->oct.mins[OCT_Y]);
      rv.oct.maxs[OCT_XY] = MIN(pv1->oct.maxs[OCT_X] + pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_X] + pv2->oct.maxs[OCT_Y]);
      if(rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY]) return rv;

      rv.oct.mins[OCT_YX] = MAX(-pv1->oct.maxs[OCT_X] + pv1->oct.mins[OCT_Y], -pv2->oct.maxs[OCT_X] + pv2->oct.mins[OCT_Y]);
      rv.oct.maxs[OCT_YX] = MIN(-pv1->oct.mins[OCT_X] + pv1->oct.maxs[OCT_Y], -pv2->oct.mins[OCT_X] + pv2->oct.maxs[OCT_Y]);
      if(rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX]) return rv;
    }

    if(0 == pv1->lod && 0 == pv2->lod)
    {
      rv.lod = 0;
    }
    else
    {
      rv.lod = MIN(pv1->lod, pv2->lod);
    }
  }

  return rv;
}

//--------------------------------------------------------------------------------------------
bool_t OVolume_refine( OVolume_t * pov, fvec3_t * pcenter, float * pvolume )
{
  // BB> determine which of the 16 possible intersection points are within both
  //     square and diamond bounding volumes

  bool_t invalid;
  int cnt, tnc, count;
  float  area, darea, volume;

  fvec3_t center, centroid;
  cv_point_data_t pd[16];

  if(NULL == pov) return bfalse;

  invalid = bfalse;
       if(pov->oct.mins[OCT_X]  >= pov->oct.maxs[OCT_X] ) invalid = btrue;
  else if(pov->oct.mins[OCT_Y]  >= pov->oct.maxs[OCT_Y] ) invalid = btrue;
  else if(pov->oct.mins[OCT_Z]  >= pov->oct.maxs[OCT_Z] ) invalid = btrue;
  else if(pov->oct.mins[OCT_XY] >= pov->oct.maxs[OCT_XY]) invalid = btrue;
  else if(pov->oct.mins[OCT_YX] >= pov->oct.maxs[OCT_YX]) invalid = btrue;

  if(invalid)
  {
    pov->lod = -1;
    if(NULL != pvolume) (*pvolume) = 0;
    return bfalse;
  }

  // square points
  cnt = 0;
  pd[cnt].pos.x = pov->oct.maxs[OCT_X];
  pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

  cnt++;
  pd[cnt].pos.x = pov->oct.maxs[OCT_X];
  pd[cnt].pos.y = pov->oct.mins[OCT_Y];

  cnt++;
  pd[cnt].pos.x = pov->oct.mins[OCT_X];
  pd[cnt].pos.y = pov->oct.mins[OCT_Y];

  cnt++;
  pd[cnt].pos.x = pov->oct.mins[OCT_X];
  pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

  // diamond points
  cnt++;
  pd[cnt].pos.x = (pov->oct.maxs[OCT_XY] - pov->oct.mins[OCT_YX]) * 0.5f;
  pd[cnt].pos.y = (pov->oct.maxs[OCT_XY] + pov->oct.mins[OCT_YX]) * 0.5f;

  cnt++;
  pd[cnt].pos.x = (pov->oct.mins[OCT_XY] - pov->oct.mins[OCT_YX]) * 0.5f;
  pd[cnt].pos.y = (pov->oct.mins[OCT_XY] + pov->oct.mins[OCT_YX]) * 0.5f;

  cnt++;
  pd[cnt].pos.x = (pov->oct.mins[OCT_XY] - pov->oct.maxs[OCT_YX]) * 0.5f;
  pd[cnt].pos.y = (pov->oct.mins[OCT_XY] + pov->oct.maxs[OCT_YX]) * 0.5f;

  cnt++;
  pd[cnt].pos.x = (pov->oct.maxs[OCT_XY] - pov->oct.maxs[OCT_YX]) * 0.5f;
  pd[cnt].pos.y = (pov->oct.maxs[OCT_XY] + pov->oct.maxs[OCT_YX]) * 0.5f;

  // intersection points
  cnt++;
  pd[cnt].pos.x = pov->oct.maxs[OCT_X];
  pd[cnt].pos.y = pov->oct.maxs[OCT_X] + pov->oct.mins[OCT_YX];

  cnt++;
  pd[cnt].pos.x = pov->oct.mins[OCT_Y] - pov->oct.mins[OCT_YX];
  pd[cnt].pos.y = pov->oct.mins[OCT_Y];

  cnt++;
  pd[cnt].pos.x =-pov->oct.mins[OCT_Y] + pov->oct.mins[OCT_XY];
  pd[cnt].pos.y = pov->oct.mins[OCT_Y];

  cnt++;
  pd[cnt].pos.x = pov->oct.mins[OCT_X];
  pd[cnt].pos.y =-pov->oct.mins[OCT_X] + pov->oct.mins[OCT_XY];

  cnt++;
  pd[cnt].pos.x = pov->oct.mins[OCT_X];
  pd[cnt].pos.y = pov->oct.mins[OCT_X] + pov->oct.maxs[OCT_YX];

  cnt++;
  pd[cnt].pos.x = pov->oct.maxs[OCT_Y] - pov->oct.maxs[OCT_YX];
  pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

  cnt++;
  pd[cnt].pos.x =-pov->oct.maxs[OCT_Y] + pov->oct.maxs[OCT_XY];
  pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

  cnt++;
  pd[cnt].pos.x = pov->oct.maxs[OCT_X];
  pd[cnt].pos.y =-pov->oct.maxs[OCT_X] + pov->oct.maxs[OCT_XY];

  // which points are outside both volumes
  fvec3_clear(&center);
  count = 0;
  for(cnt=0; cnt<16; cnt++)
  {
    float ftmp;

    pd[cnt].inside = bfalse;

    // check the box
    if(pd[cnt].pos.x < pov->oct.mins[OCT_X] || pd[cnt].pos.x > pov->oct.maxs[OCT_X]) continue;
    if(pd[cnt].pos.y < pov->oct.mins[OCT_Y] || pd[cnt].pos.y > pov->oct.maxs[OCT_Y]) continue;

    // check the diamond
    ftmp = pd[cnt].pos.x + pd[cnt].pos.y;
    if(ftmp < pov->oct.mins[OCT_XY] || ftmp > pov->oct.maxs[OCT_XY]) continue;

    ftmp =-pd[cnt].pos.x + pd[cnt].pos.y;
    if(ftmp < pov->oct.mins[OCT_YX] || ftmp > pov->oct.maxs[OCT_YX]) continue;

    // found a point
    center.x += pd[cnt].pos.x;
    center.y += pd[cnt].pos.y;
    count++;
    pd[cnt].inside = btrue;
  };

  if(count < 3) return bfalse;

  // find the centroid
  fvec3_scale(&center, 1.0f/(float)count);

  // move the valid points to the beginning of the list
  for(cnt=0, tnc=0; cnt<16 && tnc<count; cnt++)
  {
    if(!pd[cnt].inside) continue;

    // insert a valid point into the next available slot
    if(tnc != cnt)
    {
      pd[tnc] = pd[cnt];
    }

    // record the cartesian rotation angle relative to center
    pd[tnc].rads = atan2(pd[cnt].pos.y-center.y, pd[cnt].pos.x-center.x);
    tnc++;
  }

  // use qsort to order the points according to their rotation angle
  // relative to the centroid
  qsort( (void *)pd, count, sizeof( cv_point_data_t ), cv_point_data_cmp );

  // now we can use geometry to find the area of the planar collision area
  fvec3_clear(&centroid);
  {
    float ftmp;
    fvec3_t diff1, diff2;

    area = 0;
    pov->oct.mins[OCT_X]  = pov->oct.maxs[OCT_X]  = pd[0].pos.x;
    pov->oct.mins[OCT_Y]  = pov->oct.maxs[OCT_Y]  = pd[0].pos.y;
    pov->oct.mins[OCT_Z]  = pov->oct.maxs[OCT_Z]  = pd[0].pos.z;
    pov->oct.mins[OCT_XY] = pov->oct.maxs[OCT_XY] = pd[0].pos.x + pd[0].pos.y;
    pov->oct.mins[OCT_YX] = pov->oct.maxs[OCT_YX] =-pd[0].pos.x + pd[0].pos.y;
    for(cnt=0; cnt<count-1; cnt++)
    {
      tnc = cnt + 1;

      // optimize the bounding volume
      pov->oct.mins[OCT_X] = MIN(pov->oct.mins[OCT_X], pd[tnc].pos.x);
      pov->oct.maxs[OCT_X] = MAX(pov->oct.maxs[OCT_X], pd[tnc].pos.x);

      pov->oct.mins[OCT_Y] = MIN(pov->oct.mins[OCT_Y], pd[tnc].pos.y);
      pov->oct.maxs[OCT_Y] = MAX(pov->oct.maxs[OCT_Y], pd[tnc].pos.y);

      ftmp = pd[tnc].pos.x + pd[tnc].pos.y;
      pov->oct.mins[OCT_XY] = MIN(pov->oct.mins[OCT_XY], ftmp);
      pov->oct.maxs[OCT_XY] = MAX(pov->oct.maxs[OCT_XY], ftmp);

      ftmp =-pd[tnc].pos.x + pd[tnc].pos.y;
      pov->oct.mins[OCT_YX] = MIN(pov->oct.mins[OCT_YX], ftmp);
      pov->oct.maxs[OCT_YX] = MAX(pov->oct.maxs[OCT_YX], ftmp);

      // determine the area for this element
      diff1.x = pd[cnt].pos.x - center.x;
      diff1.y = pd[cnt].pos.y - center.y;

      diff2.x = pd[tnc].pos.x - pd[cnt].pos.x;
      diff2.y = pd[tnc].pos.y - pd[cnt].pos.y;

      darea = diff1.x * diff2.y - diff1.y * diff2.x;

      // esitmate the centroid
      area += darea;
      centroid.x += (pd[cnt].pos.x + pd[tnc].pos.x + center.x) / 3.0f * darea;
      centroid.y += (pd[cnt].pos.y + pd[tnc].pos.y + center.y) / 3.0f * darea;
    }

    diff1.x = pd[cnt].pos.x - center.x;
    diff1.y = pd[cnt].pos.y - center.y;

    diff2.x = pd[1].pos.x - pd[cnt].pos.x;
    diff2.y = pd[1].pos.y - pd[cnt].pos.y;

    darea = diff1.x * diff2.y - diff1.y * diff2.x;

    area += darea;
    centroid.x += (pd[cnt].pos.x + pd[1].pos.x + center.x) / 3.0f  * darea;
    centroid.y += (pd[cnt].pos.y + pd[1].pos.y + center.y) / 3.0f  * darea;
  }

  // is the volume valid?
  invalid = bfalse;
       if(pov->oct.mins[OCT_X]  >= pov->oct.maxs[OCT_X] ) invalid = btrue;
  else if(pov->oct.mins[OCT_Y]  >= pov->oct.maxs[OCT_Y] ) invalid = btrue;
  else if(pov->oct.mins[OCT_Z]  >= pov->oct.maxs[OCT_Z] ) invalid = btrue;
  else if(pov->oct.mins[OCT_XY] >= pov->oct.maxs[OCT_XY]) invalid = btrue;
  else if(pov->oct.mins[OCT_YX] >= pov->oct.maxs[OCT_YX]) invalid = btrue;

  if(invalid)
  {
    pov->lod = -1;
    if(NULL != pvolume) (*pvolume) = 0;
    return bfalse;
  }

  // determine the volume center
  if( NULL != pcenter && ABS(area) > 0)
  {
    (*pcenter).x = centroid.x / area;
    (*pcenter).y = centroid.y / area;
    (*pcenter).z = (pov->oct.maxs[OCT_Z] + pov->oct.mins[OCT_Z]) * 0.5f;
  }

  // determine the volume
  volume = ABS(area) * (pov->oct.maxs[OCT_Z] - pov->oct.mins[OCT_Z]);
  if(NULL != pvolume)
  {
    (*pvolume) = volume;
  };

  return volume > 0.0f;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t CVolume_init( OVolume_t * pva, OVolume_t * pvb, CVolume_t * pcv)
{
  bool_t retval;
  CVolume_t cv;

  if(pva->lod < 0 || pvb->lod < 0) return bfalse;

  //---- do the preliminary collision test ----

  cv.ov = OVolume_intersect(pva, pvb);
  if(cv.ov.lod < 0)
  {
    return bfalse;
  };

  //---- refine the collision volume ----

  cv.ov.lod = MIN(pva->lod, pvb->lod);
  retval = CVolume_refine( &cv );

  if(NULL != pcv)
  {
    *pcv = cv;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t CVolume_refine( CVolume_t * pcv )
{
  // BB> determine which of the 16 possible intersection points are within both
  //     square and diamond bounding volumes

  if(NULL == pcv) return bfalse;

  if(pcv->ov.oct.maxs[OCT_Z] <= pcv->ov.oct.mins[OCT_Z])
  {
    pcv->ov.lod = -1;
    pcv->volume = 0;
    return bfalse;
  }

  return OVolume_refine( &(pcv->ov), &(pcv->center), &(pcv->volume) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int cv_point_data_cmp( const void * pleft, const void * pright )
{
  int rv = 0;

  cv_point_data_t * pcv_left  = (cv_point_data_t *)pleft;
  cv_point_data_t * pcv_right = (cv_point_data_t *)pright;

  if(pcv_left->rads < pcv_right->rads) rv = -1;
  else if(pcv_left->rads > pcv_right->rads) rv = 1;

  return rv;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int oct_bb_to_points( oct_bb_t * pbmp, fvec4_t   pos[], size_t pos_count )
{
    /// @details BB@> convert the corners of the level 1 bounding box to a point cloud
    ///      set pos[].w to zero for now, that the transform does not
    ///      shift the points while transforming them
    ///
    /// @note Make sure to set pos[].w to zero so that the bounding box will not be translated
    ///      then the transformation matrix is applied.
    ///
    /// @note The math for finding the corners of this bumper is not hard, but it is easy to make a mistake.
    ///      be careful if you modify anything.

    float ftmp;
    float val_x, val_y;

    int vcount = 0;

    if ( NULL == pbmp || NULL == pos || 0 == pos_count ) return 0;

    //---- the points along the y_max edge
    ftmp = 0.5f * ( pbmp->maxs[OCT_XY] + pbmp->maxs[OCT_YX] );  // the top point of the diamond
    if ( ftmp <= pbmp->maxs[OCT_Y] )
    {
        val_x = 0.5f * ( pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_YX] );
        val_y = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_y = pbmp->maxs[OCT_Y];

        val_x = pbmp->maxs[OCT_Y] - pbmp->maxs[OCT_YX];
        if ( val_x < pbmp->mins[OCT_X] )
        {
            val_x = pbmp->mins[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_x = pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_Y];
        if ( val_x > pbmp->maxs[OCT_X] )
        {
            val_x = pbmp->maxs[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the y_min edge
    ftmp = 0.5f * ( pbmp->mins[OCT_XY] + pbmp->mins[OCT_YX] );  // the top point of the diamond
    if ( ftmp >= pbmp->mins[OCT_Y] )
    {
        val_x = 0.5f * ( pbmp->mins[OCT_XY] - pbmp->mins[OCT_YX] );
        val_y = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_y = pbmp->mins[OCT_Y];

        val_x = pbmp->mins[OCT_XY] - pbmp->mins[OCT_Y];
        if ( val_x < pbmp->mins[OCT_X] )
        {
            val_x = pbmp->mins[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_x = pbmp->mins[OCT_Y] - pbmp->mins[OCT_YX];
        if ( val_x > pbmp->maxs[OCT_X] )
        {
            val_x = pbmp->maxs[OCT_X];
        }
        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the x_max edge
    ftmp = 0.5f * ( pbmp->maxs[OCT_XY] - pbmp->mins[OCT_YX] );  // the top point of the diamond
    if ( ftmp <= pbmp->maxs[OCT_X] )
    {
        val_y = 0.5f * ( pbmp->maxs[OCT_XY] + pbmp->mins[OCT_YX] );
        val_x = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_x = pbmp->maxs[OCT_X];

        val_y = pbmp->maxs[OCT_X] + pbmp->mins[OCT_YX];
        if ( val_y < pbmp->mins[OCT_Y] )
        {
            val_y = pbmp->mins[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_y = pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_X];
        if ( val_y > pbmp->maxs[OCT_Y] )
        {
            val_y = pbmp->maxs[OCT_Y];
        }
        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the x_min edge
    ftmp = 0.5f * ( pbmp->mins[OCT_XY] - pbmp->maxs[OCT_YX] );  // the left point of the diamond
    if ( ftmp >= pbmp->mins[OCT_X] )
    {
        val_y = 0.5f * ( pbmp->mins[OCT_XY] + pbmp->maxs[OCT_YX] );
        val_x = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_x = pbmp->mins[OCT_X];

        val_y = pbmp->mins[OCT_XY] - pbmp->mins[OCT_X];
        if ( val_y < pbmp->mins[OCT_Y] )
        {
            val_y = pbmp->mins[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_y = pbmp->maxs[OCT_YX] + pbmp->mins[OCT_X];
        if ( val_y > pbmp->maxs[OCT_Y] )
        {
            val_y = pbmp->maxs[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    return vcount;
}

//--------------------------------------------------------------------------------------------
void points_to_oct_bb( oct_bb_t * pbmp, fvec4_t pos[], size_t pos_count )
{
    /// @details BB@> convert the new point cloud into a level 1 bounding box using a fvec4_t
    ///               array as the source

    Uint32 cnt;

    if ( NULL == pbmp || NULL == pos || 0 == pos_count ) return;

    // determine a bounding box for the point cloud
    pbmp->mins[OCT_X ] = pbmp->maxs[OCT_X ]  = pos[0].x;
    pbmp->mins[OCT_Y ] = pbmp->maxs[OCT_Y ]  = pos[0].y;
    pbmp->mins[OCT_Z ] = pbmp->maxs[OCT_Z ]  = pos[0].z;
    pbmp->mins[OCT_XY] = pbmp->maxs[OCT_XY] =  pos[0].x + pos[0].y;
    pbmp->mins[OCT_YX] = pbmp->maxs[OCT_YX] = -pos[0].x + pos[0].y;

    for ( cnt = 1; cnt < pos_count; cnt++ )
    {
        float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

        tmp_x = pos[cnt].x;
        pbmp->mins[OCT_X]  = MIN( pbmp->mins[OCT_X], tmp_x );
        pbmp->maxs[OCT_X]  = MAX( pbmp->maxs[OCT_X], tmp_x );

        tmp_y = pos[cnt].y;
        pbmp->mins[OCT_Y]  = MIN( pbmp->mins[OCT_Y], tmp_y );
        pbmp->maxs[OCT_Y]  = MAX( pbmp->maxs[OCT_Y], tmp_y );

        tmp_z = pos[cnt].z;
        pbmp->mins[OCT_Z]  = MIN( pbmp->mins[OCT_Z], tmp_z );
        pbmp->maxs[OCT_Z]  = MAX( pbmp->maxs[OCT_Z], tmp_z );

        tmp_xy = tmp_x + tmp_y;
        pbmp->mins[OCT_XY] = MIN( pbmp->mins[OCT_XY], tmp_xy );
        pbmp->maxs[OCT_XY] = MAX( pbmp->maxs[OCT_XY], tmp_xy );

        tmp_yx = -tmp_x + tmp_y;
        pbmp->mins[OCT_YX] = MIN( pbmp->mins[OCT_YX], tmp_yx );
        pbmp->maxs[OCT_YX] = MAX( pbmp->maxs[OCT_YX], tmp_yx );
    }
}

//--------------------------------------------------------------------------------------------
bool_t vec_to_oct_vec( fvec3_t pos, oct_vec_t ovec )
{
    if ( NULL == ovec ) return bfalse;

    ovec[OCT_X ] =  pos.x;
    ovec[OCT_Y ] =  pos.y;
    ovec[OCT_Z ] =  pos.z;
    ovec[OCT_XY] =  pos.x + pos.y;
    ovec[OCT_YX] = -pos.x + pos.y;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bumper_to_oct_bb_0( bumper_t src, oct_bb_t * pdst )
{
    if ( NULL == pdst ) return bfalse;

    pdst->mins[OCT_X] = -src.size;
    pdst->maxs[OCT_X] =  src.size;

    pdst->mins[OCT_Y] = -src.size;
    pdst->maxs[OCT_Y] =  src.size;

    pdst->mins[OCT_XY] = -src.sizebig;
    pdst->maxs[OCT_XY] =  src.sizebig;

    pdst->mins[OCT_YX] = -src.sizebig;
    pdst->maxs[OCT_YX] =  src.sizebig;

    pdst->mins[OCT_Z] = -src.height;
    pdst->maxs[OCT_Z] =  src.height;

    return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t oct_bb_add_vector( oct_bb_t src, fvec3_t vec, oct_bb_t * pdst )
{
    /// @details BB@> shift the bounding box by the vector vec

    if( NULL == pdst ) return bfalse;

    pdst->mins[OCT_X]  = src.mins[OCT_X] + vec.x;
    pdst->maxs[OCT_X]  = src.maxs[OCT_X] + vec.x;

    pdst->mins[OCT_Y]  = src.mins[OCT_Y] + vec.y;
    pdst->maxs[OCT_Y]  = src.maxs[OCT_Y] + vec.y;

    pdst->mins[OCT_XY] = src.mins[OCT_XY] + (vec.x + vec.y);
    pdst->maxs[OCT_XY] = src.maxs[OCT_XY] + (vec.x + vec.y);

    pdst->mins[OCT_YX] = src.mins[OCT_YX] + (-vec.x + vec.y);
    pdst->maxs[OCT_YX] = src.maxs[OCT_YX] + (-vec.x + vec.y);

    pdst->mins[OCT_Z]  = src.mins[OCT_Z] + vec.z;
    pdst->maxs[OCT_Z]  = src.maxs[OCT_Z] + vec.z;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t oct_bb_union( oct_bb_t src1, oct_bb_t src2, oct_bb_t * pdst )
{
    /// @details BB@> find the union of two oct_bb_t

    if( NULL == pdst ) return bfalse;

    pdst->mins[OCT_X]  = MIN(src1.mins[OCT_X],  src2.mins[OCT_X]);
    pdst->maxs[OCT_X]  = MAX(src1.maxs[OCT_X],  src2.maxs[OCT_X]);

    pdst->mins[OCT_Y]  = MIN(src1.mins[OCT_Y],  src2.mins[OCT_Y]);
    pdst->maxs[OCT_Y]  = MAX(src1.maxs[OCT_Y],  src2.maxs[OCT_Y]);

    pdst->mins[OCT_XY] = MIN(src1.mins[OCT_XY], src2.mins[OCT_XY]);
    pdst->maxs[OCT_XY] = MAX(src1.maxs[OCT_XY], src2.maxs[OCT_XY]);

    pdst->mins[OCT_YX] = MIN(src1.mins[OCT_YX], src2.mins[OCT_YX]);
    pdst->maxs[OCT_YX] = MAX(src1.maxs[OCT_YX], src2.maxs[OCT_YX]);

    pdst->mins[OCT_Z]  = MIN(src1.mins[OCT_Z],  src2.mins[OCT_Z]);
    pdst->maxs[OCT_Z]  = MAX(src1.maxs[OCT_Z],  src2.maxs[OCT_Z]);

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t oct_bb_intersection( oct_bb_t src1, oct_bb_t src2, oct_bb_t * pdst )
{
    /// @details BB@> find the intersection of two oct_bb_t

    if( NULL == pdst ) return bfalse;

    pdst->mins[OCT_X]  = MAX(src1.mins[OCT_X],  src2.mins[OCT_X]);
    pdst->maxs[OCT_X]  = MIN(src1.maxs[OCT_X],  src2.maxs[OCT_X]);

    pdst->mins[OCT_Y]  = MAX(src1.mins[OCT_Y],  src2.mins[OCT_Y]);
    pdst->maxs[OCT_Y]  = MIN(src1.maxs[OCT_Y],  src2.maxs[OCT_Y]);

    pdst->mins[OCT_XY] = MAX(src1.mins[OCT_XY], src2.mins[OCT_XY]);
    pdst->maxs[OCT_XY] = MIN(src1.maxs[OCT_XY], src2.maxs[OCT_XY]);

    pdst->mins[OCT_YX] = MAX(src1.mins[OCT_YX], src2.mins[OCT_YX]);
    pdst->maxs[OCT_YX] = MIN(src1.maxs[OCT_YX], src2.maxs[OCT_YX]);

    pdst->mins[OCT_Z]  = MAX(src1.mins[OCT_Z],  src2.mins[OCT_Z]);
    pdst->maxs[OCT_Z]  = MIN(src1.maxs[OCT_Z],  src2.maxs[OCT_Z]);

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t oct_bb_expand( oct_bb_t src, fvec3_t vel, float tmin, float tmax, oct_bb_t * pdst )
{
    // @details BB@> use the velocity of an object and its oct_bb_t to determine the
    //               amount of territory that an object will cover in one update

    oct_bb_t tmp_min, tmp_max;

    // determine the bounding volume at t == tmin
    if( tmin == 0.0f )
    {
        tmp_min = src;
    }
    else
    {
        fvec3_t pos_min;

        pos_min.x = vel.x * tmin;
        pos_min.y = vel.y * tmin;
        pos_min.z = vel.z * tmin;

        // adjust the bounding box to take in the position at the next step
        if( !oct_bb_add_vector( src, pos_min, &tmp_min ) ) return bfalse;
    }

    // determine the bounding volume at t == tmax
    if( tmax == 0.0f )
    {
        tmp_max = src;
    }
    else
    {
        fvec3_t pos_max;

        pos_max.x = vel.x * tmax;
        pos_max.y = vel.y * tmax;
        pos_max.z = vel.z * tmax;

        // adjust the bounding box to take in the position at the next step
        if( !oct_bb_add_vector( src, pos_max, &tmp_max ) ) return bfalse;
    }

    // determine bounding box for the range of times
    if( !oct_bb_union(tmp_min, tmp_max, pdst ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void oct_bb_downgrade( oct_bb_t * psrc_bb, bumper_t bump_base, bumper_t * pdst_bump, oct_bb_t * pdst_bb )
{
    /// @details BB@> convert a level 1 bumper to an "equivalent" level 0 bumper

    float val1, val2, val3, val4;

    // return if there is no source
    if ( NULL == psrc_bb ) return;

    //---- handle all of the pdst_bump data first
    if ( NULL != pdst_bump )
    {
        if ( 0 == bump_base.height )
        {
            pdst_bump->height = 0.0f;
        }
        else
        {
            // have to use MAX here because the height can be distorted due
            // to make object-particle interactions easier (i.e. it allows you to
            // hit a grub bug with your hands)

            pdst_bump->height = MAX( bump_base.height, psrc_bb->maxs[OCT_Z] );
        }

        if ( 0 == bump_base.size )
        {
            pdst_bump->size = 0.0f;
        }
        else
        {
            val1 = ABS( psrc_bb->mins[OCT_X] );
            val2 = ABS( psrc_bb->maxs[OCT_Y] );
            val3 = ABS( psrc_bb->mins[OCT_Y] );
            val4 = ABS( psrc_bb->maxs[OCT_Y] );
            pdst_bump->size = MAX( MAX( val1, val2 ), MAX( val3, val4 ) );
        }

        if ( 0 == bump_base.sizebig )
        {
            pdst_bump->sizebig = 0;
        }
        else
        {
            val1 =  psrc_bb->maxs[OCT_YX];
            val2 = -psrc_bb->mins[OCT_YX];
            val3 =  psrc_bb->maxs[OCT_XY];
            val4 = -psrc_bb->mins[OCT_XY];
            pdst_bump->sizebig = MAX( MAX( val1, val2 ), MAX( val3, val4 ) );
        }
    }

    //---- handle all of the pdst_bb data second
    if ( NULL != pdst_bb )
    {
        // memcpy() can fail horribly if the domains overlap
        // I don't think this should ever happen, though
        if ( pdst_bb != psrc_bb )
        {
            memcpy( pdst_bb, psrc_bb, sizeof( *pdst_bb ) );
        }

        if ( 0 == bump_base.height )
        {
            pdst_bb->mins[OCT_Z] = pdst_bb->maxs[OCT_Z] = 0.0f;
        }
        else
        {
            // handle the vertical distortion the same as above
            pdst_bb->maxs[OCT_Z] = MAX( bump_base.height, psrc_bb->maxs[OCT_Z] );
        }

        // 0 == bump_base.size is supposed to be shorthand for "this object doesn't interact
        // with anything", so we have to set all of the horizontal pdst_bb data to zero to
        // make
        if ( 0 == bump_base.size )
        {
            pdst_bb->mins[OCT_X ] = pdst_bb->maxs[OCT_X ] = 0.0f;
            pdst_bb->mins[OCT_Y ] = pdst_bb->maxs[OCT_Y ] = 0.0f;
            pdst_bb->mins[OCT_XY] = pdst_bb->maxs[OCT_XY] = 0.0f;
            pdst_bb->mins[OCT_YX] = pdst_bb->maxs[OCT_YX] = 0.0f;
        }
    }
}
